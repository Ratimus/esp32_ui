#include <esp32_ui/event_router.h>

////////////////////////////////////////////////////////////////////////////////
// MenuStack implementation
////////////////////////////////////////////////////////////////////////////////
// Add a new element on the stack
bool MenuStack::push(Element *el)
{
  menuprintf("MenuStack::push %s\n", el->label);
  if (depth >= STACK_SIZE)
  {
    return false;
  }

  if (stack[depth] != el)
  {
    stack[depth] = el;
    ++depth;
  }
  menuprintf("MenuStack::top now %s\n", top() ? top()->label : "nullptr");
  return top();
}

// Pop the top element off the stack
// NOTE: there's no check to make sure you aren't popping the only element (because maybe
// there's a valid reason you'd want to do that? I dunno, I'm not your mom), so be careful
Element *MenuStack::pop()
{
  if (depth == 0)
  {
    return nullptr;
  }

  menuprintf("MenuStack::pop, removing %s\n", top() ? top()->label : "nullptr");
  --depth;
  stack[depth] = nullptr;
  return top();
}

// Get a pointer to the top element of the stack
Element *MenuStack::top() const
{
  if (depth > 0)
  {
    return stack[depth - 1];
  }
  return nullptr;
}

// Get a pointer to the bottom element of the stack
Element *MenuStack::root() const
{
  if (depth > 0)
  {
    return stack[0];
  }
  return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
// EventRouter implementation
////////////////////////////////////////////////////////////////////////////////
EventRouter *EventRouter::instance()
{
  static EventRouter inst;
  return &inst;
}

// Tell all the active elements that they need to sync their data
void EventRouter::request_sync()
{
  std::lock_guard<std::recursive_mutex> lock(stack_mutex);
  sync_pending = true;
}

// Temporarily route all MenuEvents of a given source and index to a given element
bool EventRouter::bind_popup(MenuEvent::Source src, uint8_t idx, Element *el)
{
  std::lock_guard<std::recursive_mutex> lock(stack_mutex);
  bindings[1][{src, idx}] = el;
  return true;
}

// Remove temporary routing of MenuEvents for a given source and index
bool EventRouter::unbind_popup(MenuEvent::Source src, uint8_t idx)
{
  std::lock_guard<std::recursive_mutex> lock(stack_mutex);
  bindings[1].erase({src, idx});
  return true;
}

// Sets up filter in dispatcher to divert events to a specific target
bool EventRouter::bind(MenuEvent::Source src, uint8_t idx, Element *el)
{
  std::lock_guard<std::recursive_mutex> lock(stack_mutex);
  el->register_event_listener(MenuEvent{src, MenuEvent::Type::AnyAndAll, idx});
  bindings[0][{src, idx}] = el;
  return true;
}

// Stops filtering out specific events pre-dispatch
bool EventRouter::unbind(MenuEvent::Source source, uint8_t idx)
{
  std::lock_guard<std::recursive_mutex> lock(stack_mutex);
  Element *el = static_cast<Element *>(bindings[0][{source, idx}]);
  el->unregister_event_listener(MenuEvent{source, MenuEvent::Type::AnyAndAll, idx});
  bindings[0].erase({source, idx});
  return true;
}

// To dispatch an event, send it to somebody's handle_event(). If it returns true,don't
// send it to anybody else. To that end, every event handler should return true unless
// something bad happens.
void EventRouter::dispatch(const MenuEvent &ev)
{
  Element *top = nullptr;
  {
    std::lock_guard<std::recursive_mutex> lock(stack_mutex);
    top = menu_stack.top();
  }

  if (!top)
  {
    return;
  }

  if (ev.type == MenuEvent::Type::Sync)
  {
    sync_pending = true;
  }

  if (sync_pending)
  {
    top->handle_event(MenuEvent{MenuEvent::Source::System, MenuEvent::Type::Sync, 0});
    sync_pending = false;
  }

  if (ev.type == MenuEvent::Type::Draw)
  {
    auto *d = Display::instance();
    d->clearBuffer();
    top->handle_draw(d);
    d->sendBuffer();
    return;
  }

  if (handle_temporary_interceptors(ev))
  {
    return;
  }

  if (handle_hardwired_interceptors(ev))
  {
    return;
  }

  if (top->is_schleep())
  {
    top->wake_up();
    return;
  }

  // Route everything else to the active Element and let it sort things out
  if (top->handle_event(ev))
  {
    return;
  }

  if (default_handler)
  {
    // If you want to do a bunch of work, you can make dispatch_event() return a bool
    // and return if it's successful, otherwise call the default (if have one registered).
    default_handler(ev);
  }
}

// Push the an Element to the top of the stack
bool EventRouter::push_menu(Element *el)
{
  menuprintf("push_menu: %s\n", el->label);
  assert((el != nullptr) && "null ptr in EventRouter::push_menu");
  if (top_menu() == el)
  {
    // Don't double-push the same menu
    return false;
  }

  if (menu_stack.full())
  {
    return false;
  }

  if (top_menu())
  {
    top_menu()->handle_exit();
  }
  menu_stack.push(el);

  top_menu()->handle_enter();
  return true;
}

//
bool EventRouter::pop_menu()
{
  std::lock_guard<std::recursive_mutex> lock(stack_mutex);
  if (menu_stack.empty())
  {
    return false;
  }

  if (top_menu() == root_menu())
  {
    top_menu()->go_schleep();
    return true;
  }

  top_menu()->handle_exit();

  menu_stack.pop();
  assert(top_menu());
  top_menu()->handle_enter();

  return true;
}

Element *EventRouter::top_menu() const
{
  std::lock_guard<std::recursive_mutex> lock(stack_mutex);
  return menu_stack.top();
}

Element *EventRouter::root_menu() const
{
  std::lock_guard<std::recursive_mutex> lock(stack_mutex);
  return menu_stack.root();
}

// This one will navigate to a new screen, but will navigate
// to the *previous* screen when the new screen exits.
// E.G. from MenuA, navigate to MenuB. When exiting MenuB,
// rather than just calling pop() call overwrite_top(&popup_menu).
// You can do a Save?<yes>:no screen or something before navigating
// backward.
Element *EventRouter::overwrite_top(Element *el)
{
  std::lock_guard<std::recursive_mutex> lock(stack_mutex);
  Element *popped = nullptr;
  if (top_menu())
  {
    top_menu()->handle_exit();
    popped = menu_stack.pop();
  }

  menu_stack.push(el);
  el->handle_enter();

  return popped;
}

bool EventRouter::handle_hardwired_interceptors(const MenuEvent &ev)
{
  // Filter out and route explicit bindings (hardwired or temporary override)
  auto it = bindings[0].find({ev.source, ev.index});
  if (it != bindings[0].end() && it->second)
  {
    return it->second->handle_event(ev);
  }
  return false;
}

bool EventRouter::handle_temporary_interceptors(const MenuEvent &ev)
{
  auto tmp = bindings[1].find({ev.source, ev.index});
  if (tmp != bindings[1].end() && tmp->second)
  {
    auto *target = tmp->second;
    return target->handle_event(ev);
  }
  return false;
}
