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
  mutable std::recursive_mutex stack_mutex;
  bool sync_pending = false;

public:
  // Meyers singleton
  static EventRouter *instance();

  void request_sync();
  void dispatch(const MenuEvent &ev);

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

//  ChatGPT sez:
// Overview: What's in Your EventRouter
//     It’s well-structured, modular, and thread-safe (via std::recursive_mutex)
//     Built for flexible UI routing: menus, interceptors, popups, draw sync, etc.
//     Uses std::unordered_map<Key, MenuBase*> for two layers of bindings
//     Light stack implementation with MenuStack and fixed STACK_SIZE = 8
//     Dispatch logic is clean, with good early exits
// ⚡ Speed/Performance Bottlenecks
// The only parts of this code that might cause any significant runtime overhead on a microcontroller like the ESP32 are:
// 1. std::recursive_mutex
// Recursive mutexes aren’t free. If dispatch happens frequently (e.g. every input or every 1ms tick), locking can slow things down:
//     ⚠️ Cost: On ESP32, std::recursive_mutex maps to a FreeRTOS recursive mutex under the hood. Entering/leaving a critical section takes time — especially if it's uncontended but frequent.
//     ✅ Mitigation: Only lock around actual data changes; consider whether full top_menu() calls really need mutex (e.g. menu_stack.top() is likely safe if reads are atomic and write race is acceptable).
// 2. std::unordered_map on embedded targets
//     ⚠️ Cost: unordered_map is hash-based and can allocate on the heap. ESP32 has limited RAM, and collisions can cause linear scan of buckets.
//     🧠 Better Alternative: On embedded systems, replace std::unordered_map with a fixed-size hash table or flat map if your keyspace is small and bounded.
// Example:
// static constexpr int MAX_BINDINGS = 8;
// std::array<Key, MAX_BINDINGS> keys;
// std::array<Element*, MAX_BINDINGS> elements;
// Then just do linear search — if performance is comparable and memory usage is known, this is often faster and more predictable.
// 3. Function pointers and virtual dispatch
//     You call el->handle_event() often
//     If Element uses virtual functions, it incurs indirect calls, which are ~30–50 cycles on ESP32
//     Not a problem unless you're calling these hundreds/thousands of times per second
// 🚀 Optimization Targets (by impact)
// Area	Gain	Strategy
// unordered_map → flat array	✅ High	Replace with small flat-table or static lookup if number of bindings is predictable
// recursive_mutex	⚠️ Medium	Use portENTER_CRITICAL() only when needed, or replace with std::mutex if recursion isn't required
// Memory layout	⚠️ Medium	Reduce pointer chasing (e.g. flatten Element if access patterns are static)
// dispatch() logic	✅ Low	Already tight. Might benefit from branch prediction hints ([[likely]] / __builtin_expect) if GCC
// Stack depth (MenuStack)	⚠️ Low	Rarely changes, and small size (8) means cache hits are likely. No action needed.
// ✅ Profiling First
// Before doing any of the above blindly, profile with:
//     Cycle counters: use esp_timer_get_time() before/after dispatch()
//     Heap tracking: watch for allocations from unordered_map
//     Stack overflow: enable configCHECK_FOR_STACK_OVERFLOW in FreeRTOS
// ✅ TL;DR Answer
//     ❓ Can you speed this up much?
//     Yes, a bit. But this code is not your bottleneck unless you’re dispatching thousands of events per second.
// 🔧 Best quick win?
//     Replace unordered_map with a simple flat lookup table
//     Switch to std::mutex if recursive_mutex isn’t needed
//     Avoid holding the lock longer than necessary in top_menu(), root_menu()
//

//   // To dispatch an event, send it to somebody's handle_event(). If it returns true,don't
//   // send it to anybody else. To that end, every event handler should return true unless
//   // something bad happens.
//   void dispatch(const MenuEvent &ev)
//   {
//     std::lock_guard<std::mutex> lock(stack_mutex);
//     Element *top = menu_stack.top(); // Flattened from top_menu()
//     if (!top)
//     {
//       return;
//     }
//     if (ev.type == MenuEvent::Type::Sync)
//     {
//       sync_pending = true;
//     }
//     if (sync_pending)
//     {
//       top->handle_event(MenuEvent{MenuEvent::Source::System, MenuEvent::Type::Sync, 0});
//       sync_pending = false;
//     }
//     if (ev.type == MenuEvent::Type::Draw)
//     {
//       auto *d = Display::instance();
//       d->clearBuffer();
//       top->handle_draw(d);
//       d->sendBuffer();
//       return;
//     }
//     if (handle_temporary_interceptors(ev))
//     {
//       return;
//     }
//     if (handle_hardwired_interceptors(ev))
//     {
//       return;
//     }
//     if (top->is_schleep())
//     {
//       top->wake_up();
//       return;
//     }
//     if (top->handle_event(ev))
//     {
//       return;
//     }
//     if (default_handler)
//     {
//       default_handler(ev);
//     }
//   }
//   bool push_menu(Element *el)
//   {
//     assert((el != nullptr) && "null ptr in EventRouter::push_menu");
//     std::lock_guard<std::mutex> lock(stack_mutex);
//     Element *top = menu_stack.top();
//     if (top == el || menu_stack.full())
//     {
//       return false;
//     }
//     if (top)
//     {
//       top->handle_exit();
//     }
//     menu_stack.push(el);
//     el->handle_enter();
//     return true;
//   }
//   bool pop_menu()
//   {
//     std::lock_guard<std::mutex> lock(stack_mutex);
//     if (menu_stack.empty())
//     {
//       return false;
//     }
//     Element *top = menu_stack.top();
//     Element *root = menu_stack.root();
//     if (top == root)
//     {
//       top->go_schleep();
//       return true;
//     }
//     top->handle_exit();
//     menu_stack.pop();
//     Element *new_top = menu_stack.top();
//     assert(new_top);
//     new_top->handle_enter();
//     return true;
//   }
//   Element *overwrite_top(Element *el)
//   {
//     std::lock_guard<std::mutex> lock(stack_mutex);
//     Element *popped = nullptr;
//     Element *top = menu_stack.top();
//     if (top)
//     {
//       top->handle_exit();
//       popped = menu_stack.pop();
//     }
//     menu_stack.push(el);
//     el->handle_enter();
//     return popped;
//   }
// private:
//   struct Key
//   {
//     MenuEvent::Source source;
//     uint8_t index;
//     bool operator==(const Key &other) const
//     {
//       return source == other.source && index == other.index;
//     }
//   };
