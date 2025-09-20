#include "modules/ChatHistoryStore.h"
#include <algorithm>
#include "FSCommon.h"
#include <sstream>

namespace chat {


static const std::deque<ChatEntry> kEmptyDeque;

// --- Serialización CSV simple ---
std::string ChatEntry::serialize(const ChatEntry& e) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%u,%d,%d,%u,%u,", e.ts, e.outgoing, e.isChannel, e.node, e.channel);
  std::string s(buf);
  // Escapar comas en el texto si es necesario (simple)
  for (char c : e.text) {
    if (c == ',') s += "<c>";
    else s += c;
  }
  return s;
}

ChatEntry ChatEntry::deserialize(const std::string& line) {
  ChatEntry e;
  try {
    std::stringstream ss(line);
    std::string item;
    if (!std::getline(ss, item, ',')) return e;
    e.ts = std::stoul(item);
    if (!std::getline(ss, item, ',')) return e; 
    e.outgoing = std::stoi(item);
    if (!std::getline(ss, item, ',')) return e;
    e.isChannel = std::stoi(item);
    if (!std::getline(ss, item, ',')) return e;
    e.node = std::stoul(item);
    if (!std::getline(ss, item, ',')) return e;
    e.channel = std::stoul(item);
    if (!std::getline(ss, item)) return e; // resto es texto
    
    // Desescapar comas
    size_t pos = 0, last = 0;
    std::string txt;
    while ((pos = item.find("<c>", last)) != std::string::npos) {
      txt += item.substr(last, pos - last) + ',';
      last = pos + 3;
    }
    txt += item.substr(last);
    e.text = txt;
  } catch (...) {
    // Si hay error en el parsing, devolver entrada vacía
    e = ChatEntry{};
  }
  return e;
}


ChatHistoryStore& ChatHistoryStore::instance() {
  static ChatHistoryStore inst;
  return inst;
}

ChatHistoryStore::ChatHistoryStore() {
  // No cargar datos síncronamente en el constructor para evitar bucles de reinicio
  // La carga se hará bajo demanda
}

void ChatHistoryStore::pushBounded(std::deque<ChatEntry>& q, ChatEntry e) {
  // Insert in chronological order (ascending timestamp)
  if (q.empty() || q.back().ts <= e.ts) {
    q.push_back(std::move(e));
  } else {
    auto it = std::upper_bound(q.begin(), q.end(), e.ts,
      [](uint32_t t, const ChatEntry& ce){ return t < ce.ts; });
    q.insert(it, std::move(e));
  }
  while (q.size() > kMaxPerGroup) q.pop_front();
}

void ChatHistoryStore::addDM(uint32_t peer, bool outgoing, const std::string& text, uint32_t ts) {
  ChatEntry e;
  e.ts       = ts;
  e.outgoing = outgoing;
  e.isChannel = false;
  e.node     = peer;     // peer of the conversation
  e.channel  = 0;
  e.text     = text;
  pushBounded(dm_[peer], std::move(e));
  saveDM(peer);
}

void ChatHistoryStore::addCHAN(uint8_t channel, uint32_t fromNode, bool outgoing, const std::string& text, uint32_t ts) {
  ChatEntry e;
  e.ts       = ts;
  e.outgoing = outgoing;
  e.isChannel = true;
  e.node     = fromNode;   // sender (for alias display); 0 if it's us and doesn't matter
  e.channel  = channel;
  e.text     = text;
  pushBounded(ch_[channel], std::move(e));
  saveCHAN(channel);
}
// --- Persistencia ---
void ChatHistoryStore::saveDM(uint32_t peer) {
  std::string filename = "/chat_dm_" + std::to_string(peer) + ".txt";
  auto f = FSCom.open(filename.c_str(), FILE_O_WRITE);
  if (!f) return;
  for (const auto& e : dm_[peer]) {
    f.println(ChatEntry::serialize(e).c_str());
  }
  f.close();
}

void ChatHistoryStore::loadDM(uint32_t peer) {
  std::string filename = "/chat_dm_" + std::to_string(peer) + ".txt";
  auto f = FSCom.open(filename.c_str(), FILE_O_READ);
  if (!f) return; // Archivo no existe, sin error
  
  std::deque<ChatEntry> q;
  try {
    while (f.available()) {
      std::string line = f.readStringUntil('\n').c_str();
      if (!line.empty() && line.length() < 512) { // Validación básica de tamaño
        ChatEntry entry = ChatEntry::deserialize(line);
        // Validación básica de datos
        if (entry.ts > 0 && entry.ts < 4000000000U && entry.text.length() < 256) {
          q.push_back(std::move(entry));
        }
      }
    }
    dm_[peer] = std::move(q);
  } catch (...) {
    // Si hay error en la deserialización, ignora el archivo
    dm_[peer] = std::deque<ChatEntry>();
  }
  f.close();
}

void ChatHistoryStore::saveCHAN(uint8_t channel) {
  std::string filename = "/chat_ch_" + std::to_string(channel) + ".txt";
  auto f = FSCom.open(filename.c_str(), FILE_O_WRITE);
  if (!f) return;
  for (const auto& e : ch_[channel]) {
    f.println(ChatEntry::serialize(e).c_str());
  }
  f.close();
}

void ChatHistoryStore::loadCHAN(uint8_t channel) {
  std::string filename = "/chat_ch_" + std::to_string(channel) + ".txt";
  auto f = FSCom.open(filename.c_str(), FILE_O_READ);
  if (!f) return; // Archivo no existe, sin error
  
  std::deque<ChatEntry> q;
  try {
    while (f.available()) {
      std::string line = f.readStringUntil('\n').c_str();
      if (!line.empty() && line.length() < 512) { // Validación básica de tamaño
        ChatEntry entry = ChatEntry::deserialize(line);
        // Validación básica de datos
        if (entry.ts > 0 && entry.ts < 4000000000U && entry.text.length() < 256) {
          q.push_back(std::move(entry));
        }
      }
    }
    ch_[channel] = std::move(q);
  } catch (...) {
    // Si hay error en la deserialización, ignora el archivo
    ch_[channel] = std::deque<ChatEntry>();
  }
  f.close();
}

void ChatHistoryStore::saveAll() {
  for (const auto& kv : dm_) saveDM(kv.first);
  for (const auto& kv : ch_) saveCHAN(kv.first);
}

void ChatHistoryStore::loadAll() {
  // NO cargar agresivamente al inicio para evitar bucles de reinicio
  // La carga se hace bajo demanda cuando se necesite cada conversación
  // Esta función queda por compatibilidad pero no hace nada crítico
}

const std::deque<ChatEntry>& ChatHistoryStore::getDM(uint32_t peer) const {
  auto it = dm_.find(peer);
  if (it != dm_.end()) return it->second;
  
  // Carga bajo demanda con manejo de errores
  try {
    const_cast<ChatHistoryStore*>(this)->loadDM(peer);
    it = dm_.find(peer);
    if (it != dm_.end()) return it->second;
  } catch (...) {
    // Si falla la carga, devolver deque vacío silenciosamente
  }
  
  return kEmptyDeque;
}

const std::deque<ChatEntry>& ChatHistoryStore::getCHAN(uint8_t channel) const {
  auto it = ch_.find(channel);
  if (it != ch_.end()) return it->second;
  
  // Carga bajo demanda con manejo de errores  
  try {
    const_cast<ChatHistoryStore*>(this)->loadCHAN(channel);
    it = ch_.find(channel);
    if (it != ch_.end()) return it->second;
  } catch (...) {
    // Si falla la carga, devolver deque vacío silenciosamente
  }
  
  return kEmptyDeque;
}

void ChatHistoryStore::clearDM(uint32_t peer) {
  dm_.erase(peer);
}

void ChatHistoryStore::clearCHAN(uint8_t channel) {
  ch_.erase(channel);
}

void ChatHistoryStore::removeByNode(uint32_t peer) {
  dm_.erase(peer);
}

void ChatHistoryStore::removeChannel(uint8_t channel) {
  ch_.erase(channel);
}

// Nuevas funciones para eliminar solo el historial pero mantener canal/frame
void ChatHistoryStore::clearChatHistoryDM(uint32_t peer) {
  // Eliminar de RAM
  dm_.erase(peer);
  
  // Eliminar archivo persistente
  std::string filename = "/chat_dm_" + std::to_string(peer) + ".txt";
  FSCom.remove(filename.c_str());
}

void ChatHistoryStore::clearChatHistoryChannel(uint8_t channel) {
  // Eliminar de RAM
  ch_.erase(channel);
  
  // Eliminar archivo persistente
  std::string filename = "/chat_ch_" + std::to_string(channel) + ".txt";
  FSCom.remove(filename.c_str());
}

std::vector<uint32_t> ChatHistoryStore::listDMPeers() const {
  std::vector<uint32_t> v;
  v.reserve(dm_.size());
  for (auto& kv : dm_) v.push_back(kv.first); // Collect all DM peer IDs
  std::sort(v.begin(), v.end());
  return v;
}

std::vector<uint8_t> ChatHistoryStore::listChannels() const {
  std::vector<uint8_t> v;
  v.reserve(ch_.size());
  for (auto& kv : ch_) v.push_back(kv.first); // Collect all channel indices
  std::sort(v.begin(), v.end());
  return v;
}

} // namespace chat
