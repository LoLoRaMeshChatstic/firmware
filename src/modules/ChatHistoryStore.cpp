#include "modules/ChatHistoryStore.h"
#include <algorithm>

namespace chat {

static const std::deque<ChatEntry> kEmptyDeque;

ChatHistoryStore& ChatHistoryStore::instance() {
  static ChatHistoryStore inst;
  return inst;
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
}

const std::deque<ChatEntry>& ChatHistoryStore::getDM(uint32_t peer) const {
  auto it = dm_.find(peer);
  if (it != dm_.end()) return it->second;
  return kEmptyDeque; // Return empty deque if peer not found
}

const std::deque<ChatEntry>& ChatHistoryStore::getCHAN(uint8_t channel) const {
  auto it = ch_.find(channel);
  if (it != ch_.end()) return it->second;
  return kEmptyDeque; // Return empty deque if channel not found
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
