-- state_machine.lua
-- this isolates generic state/transition logic so enemies stay small and readable.
-- the api favors clarity over cleverness, and transitions are data-driven to cut boilerplate.

local StateMachine = {}
StateMachine.__index = StateMachine

-- constructor
-- keep core timing (elapsed_time_in_state) inside the machine so states can make time-based decisions.
function StateMachine.new(initial_state_id)
   return setmetatable({
      current_state_id          = initial_state_id,  -- current logical state id
      previous_state_id         = initial_state_id,  -- last state id (useful for debugging)
      elapsed_time_in_state     = 0.0,               -- seconds since the current state was entered

      registered_states         = {},   -- state_id -> { on_enter?, on_update?, on_exit? }
      transition_rules_by_state = {},   -- from_state_id -> array of { to=state_id, when=guard_fn }
      global_transition_rules   = {},   -- array of { to=state_id, when=guard_fn } checked after local rules
   }, StateMachine)
end

-- register or replace a state
-- we keep per-state side effects small and push most branching into transition rules.
function StateMachine:add_state(state_id, handlers)
   self.registered_states[state_id] = {
      on_enter  = handlers and handlers.on_enter  or nil,  -- called right after entering
      on_update = handlers and handlers.on_update or nil,  -- called every frame
      on_exit   = handlers and handlers.on_exit   or nil,  -- called right before leaving
   }
   return self
end

-- add a single transition rule with an optional guard
-- rules are evaluated in order; use this to express priority without adding code in states.
function StateMachine:add_transition_rule(from_state_id, to_state_id, when_fn)
   local rules = self.transition_rules_by_state[from_state_id]
   if not rules then
      rules = {}
      self.transition_rules_by_state[from_state_id] = rules
   end
   rules[#rules + 1] = { to = to_state_id, when = when_fn or function(_) return true end }
   return self
end

-- convenience to add multiple rules from one state
function StateMachine:add_transition_rules(from_state_id, rule_list)
   for _, rule in ipairs(rule_list) do
      if type(rule) == "table" then
         self:add_transition_rule(from_state_id, rule.to, rule.when)
      else
         self:add_transition_rule(from_state_id, rule, nil)
      end
   end
   return self
end

-- add a global rule (checked for every state)
-- good for “always” conditions like death without sprinkling checks into every state.
function StateMachine:add_global_rule(to_state_id, when_fn)
   self.global_transition_rules[#self.global_transition_rules + 1] =
      { to = to_state_id, when = when_fn or function(_) return true end }
   return self
end

-- ask whether a transition is permitted right now (guards included)
function StateMachine:can_transition(from_state_id, to_state_id)
   local rules = self.transition_rules_by_state[from_state_id]
   if rules then
      for _, rule in ipairs(rules) do
         if rule.to == to_state_id and rule.when(self) then
            return true
         end
      end
   end
   for _, rule in ipairs(self.global_transition_rules) do
      if rule.to == to_state_id and rule.when(self) then
         return true
      end
   end
   return false
end

-- internal enter without guard check
-- we centralize enter/exit timing and callbacks to avoid duplicated reset code in users.
function StateMachine:_enter_unchecked(next_state_id)
   if self.current_state_id ~= next_state_id then
      local current_handlers = self.registered_states[self.current_state_id]
      if current_handlers and current_handlers.on_exit then
         current_handlers.on_exit(self)  -- let the old state clean up
      end
      self.previous_state_id     = self.current_state_id
      self.current_state_id      = next_state_id
      self.elapsed_time_in_state = 0.0
      local next_handlers = self.registered_states[next_state_id]
      if next_handlers and next_handlers.on_enter then
         next_handlers.on_enter(self)    -- let the new state initialize
      end
   else
      -- re-entering same state: reset timer and allow optional re-init
      self.elapsed_time_in_state = 0.0
      local same_handlers = self.registered_states[next_state_id]
      if same_handlers and same_handlers.on_enter then
         same_handlers.on_enter(self)
      end
   end
end

-- guarded enter
function StateMachine:enter_state(next_state_id)
   if self:can_transition(self.current_state_id, next_state_id) then
      self:_enter_unchecked(next_state_id)
   end
end

-- advance time and evaluate transitions
-- order of decisions is: explicit proposal from state -> local rules -> global rules.
-- this keeps intent obvious and prevents “hidden” transitions from surprising us.
function StateMachine:advance_time(delta_time)
   self.elapsed_time_in_state = self.elapsed_time_in_state + delta_time

   local handlers = self.registered_states[self.current_state_id]
   local proposed_next_state = nil
   if handlers and handlers.on_update then
      proposed_next_state = handlers.on_update(delta_time, self)
   end

   local function try_switch(to_state_id)
      if to_state_id and self:can_transition(self.current_state_id, to_state_id) then
         self:_enter_unchecked(to_state_id)
         return true
      end
      return false
   end

   -- prefer explicit proposal from the state
   if try_switch(proposed_next_state) then return end

   -- then check local rules in declared order
   local local_rules = self.transition_rules_by_state[self.current_state_id]
   if local_rules then
      for _, rule in ipairs(local_rules) do
         if rule.when(self) and try_switch(rule.to) then
            return
         end
      end
   end

   -- finally, global rules (lowest priority)
   for _, rule in ipairs(self.global_transition_rules) do
      if rule.when(self) and try_switch(rule.to) then
         return
      end
   end
end

return StateMachine
