#pragma once
#include <deque>
#include <map>
#include <string>
#include <vector>
#include <stdint.h>

namespace chat {

/**
 * Entrada de historial de chat.
 * - Para DM: isChannel=false, 'node' es el peer y 'channel' es 0.
 * - Para Canal: isChannel=true, 'channel' es el índice del canal y 'node' es el remitente (0 si no disponible).
 */
struct ChatEntry {
  uint32_t ts{0};           // epoch segundos
  bool     outgoing{false}; // true si lo enviaste desde este nodo
  bool     isChannel{false}; // true=canal, false=DM por nodo
  bool     unread{false};   // true si el mensaje no ha sido leído
  uint32_t node{0};         // DM: peer; Canal: nodeId del remitente (0 si no disponible)
  uint8_t  channel{0};      // válido si isChannel==true
  std::string text;         // UTF-8 renderizable en OLED

  // Serialización simple CSV
  static std::string serialize(const ChatEntry& e);
  static ChatEntry deserialize(const std::string& line);
};

class ChatHistoryStore {
public:
  static ChatHistoryStore& instance();

  // Añadir mensajes
  void addDM(uint32_t peer, bool outgoing, const std::string& text, uint32_t ts, bool unread = true);
  void addCHAN(uint8_t channel, uint32_t fromNode, bool outgoing, const std::string& text, uint32_t ts, bool unread = true);

  // Acceso de sólo lectura al historial (devuelven un deque estable; vacío si no existe)
  const std::deque<ChatEntry>& getDM(uint32_t peer) const;
  const std::deque<ChatEntry>& getCHAN(uint8_t channel) const;

  // Gestión
  void clearDM(uint32_t peer);
  void clearCHAN(uint8_t channel);
  void removeByNode(uint32_t peer);     // borra toda la conversación DM con ese peer
  void removeChannel(uint8_t channel);  // borra todo el historial del canal

  // Nuevos métodos para eliminar historial completo (RAM + persistente) pero mantener canal/frame
  void clearChatHistoryDM(uint32_t peer);        // Elimina solo historial DM, mantiene el peer
  void clearChatHistoryChannel(uint8_t channel); // Elimina solo historial canal, mantiene canal/frame

  // Gestión de mensajes no leídos
  int getUnreadCountDM(uint32_t peer) const;     // Cuenta mensajes no leídos de un DM específico
  int getUnreadCountCHAN(uint8_t channel) const; // Cuenta mensajes no leídos de un canal específico
  int getTotalUnreadCount() const;               // Cuenta total de mensajes no leídos
  void markAsReadDM(uint32_t peer);              // Marca todos los mensajes DM como leídos
  void markAsReadCHAN(uint8_t channel);          // Marca todos los mensajes del canal como leídos
  void markAllAsRead();                          // Marca todos los mensajes como leídos
  void markMessageAsRead(uint32_t peer, int messageIndex); // Marca mensaje específico DM como leído
  void markChannelMessageAsRead(uint8_t channel, int messageIndex); // Marca mensaje específico del canal como leído
  
  // Funciones para posicionar marquee en primer mensaje no leído
  int getFirstUnreadIndexDM(uint32_t peer) const;       // Retorna índice del primer mensaje no leído en DM (-1 si todos leídos)
  int getFirstUnreadIndexCHAN(uint8_t channel) const;   // Retorna índice del primer mensaje no leído en canal (-1 si todos leídos)
  int getLastReadIndexDM(uint32_t peer) const;          // Retorna índice del último mensaje leído en DM (-1 si ninguno leído)  
  int getLastReadIndexCHAN(uint8_t channel) const;      // Retorna índice del último mensaje leído en canal (-1 si ninguno leído)

  // Listados
  std::vector<uint32_t> listDMPeers() const;
  std::vector<uint8_t>  listChannels() const;

  // Límite por conversación/canal
  static constexpr size_t kMaxPerGroup = 15;

private:
  ChatHistoryStore();
  static void pushBounded(std::deque<ChatEntry>& q, ChatEntry e);

  void saveDM(uint32_t peer);
  void loadDM(uint32_t peer);
  void saveCHAN(uint8_t channel);
  void loadCHAN(uint8_t channel);
  void saveAll();
  void loadAll();

  std::map<uint32_t, std::deque<ChatEntry>> dm_;
  std::map<uint8_t , std::deque<ChatEntry>> ch_;
};

} // namespace chat
