#pragma once

#include <unordered_map>
#include <functional>
#include <mutex>

#include <esp32_ui/menu_event.h>
#include <esp32_ui/menu_base.h>
#include <esp32_ui/element.h>

/*
Use bind() for:
  Hardwired controls with global behavior
  Modal/popup editors that should intercept all input

Use MenuStack for:
  Navigating between MenuNodes and FieldNodes
  Routing context-sensitive input like <back>, <select>, encoder rotations
  Fallback event handler
*/

namespace esp32_ui
{
  class MenuStack
  {
  public:
    bool push(Element *el);
    Element *pop();
    Element *top() const;
    Element *root() const;

    size_t size() const { return depth; }
    void clear() { depth = 0; }
    bool empty() const { return depth == 0; }

    size_t max_depth() const { return STACK_SIZE; }
    bool full() const { return depth >= STACK_SIZE; }

    MenuStack() = default;

  private:
    inline static constexpr size_t STACK_SIZE = 8;
    std::array<Element *, STACK_SIZE> stack = {nullptr};
    size_t depth = 0;
  };

  class EventRouter
  {
    MenuStack menu_stack;
    mutable std::mutex stack_mutex;
    bool sync_pending = false;

    // Limit access; call UIManager::dispatch_event(MenuEvent ev) to place
    // events on queue
    void dispatch(const MenuEvent &ev);
    friend void evt_dispatch_task(void * param);

  public:
    // Meyers singleton
    static EventRouter *instance();

    bool bind_popup(MenuEvent::Source src, uint8_t idx, Element *el);
    bool unbind_popup(MenuEvent::Source src, uint8_t idx);
    bool bind(MenuEvent::Source src, uint8_t idx, Element *el);
    bool unbind(MenuEvent::Source source, uint8_t idx);

    bool push_menu(Element *el);
    bool pop_menu();

    Element *top_menu() const;
    Element *root_menu() const;
    Element *overwrite_top(Element *el);

  private:
    struct Key
    {
      MenuEvent::Source source;
      uint8_t index;

      bool operator==(const Key &other) const
      {
        return source == other.source && index == other.index;
      }
    };

    struct KeyHash
    {
      std::size_t operator()(const Key &k) const
      {
        return std::hash<int>()(static_cast<int>(k.source)) ^ std::hash<uint8_t>()(k.index << 4);
      }
    };

    std::array<std::unordered_map<Key, MenuBase *, KeyHash>, 2> bindings;
    std::function<void(MenuEvent)> default_handler;

    bool handle_hardwired_interceptors(const MenuEvent &ev);
    bool handle_temporary_interceptors(const MenuEvent &ev);
    void set_default_handler(std::function<void(MenuEvent)> handler)
    {
      default_handler = std::move(handler);
    }
  };

} // namespace esp32_ui
