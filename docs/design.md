# Finite Automata Library and Explorer Design

## Purpose

Build a small C++ library, `libfsm`, and a Qt desktop application for creating,
viewing, saving, loading, and stepping through deterministic and
nondeterministic finite automata.

The library owns automaton semantics and execution. The application presents
those semantics graphically and provides editing and playback. Keeping this
boundary clear allows the library to be tested and reused without a GUI.

## Goals

- Represent DFAs and NFAs, including epsilon transitions and incomplete
  transition sets.
- Execute an input string and expose enough trace data to explain each step.
- Edit automata visually, with stable state and transition identities.
- Load and save a documented, versioned file format.
- Present a canonical mathematical view of an automaton regardless of its
  internal storage strategy.
- Build the features as small, independently testable increments.

## Not in the first version

- Conversion between DFA and NFA, determinization, minimization, or regular
  expression conversion.
- Pushdown automata, transducers, weighted automata, or infinite alphabets.
- Collaborative editing, plug-ins, or a general-purpose graph editor.
- A requirement that a DFA be total. Missing transitions have defined
  rejecting behavior; a future UI feature may offer totalization explicitly.

## Terminology and semantics

An automaton has a finite set of states `Q`, an alphabet `Σ`, an initial state
`q0`, a set of accepting states `F`, and transitions. State identity is a
stable, opaque ID; display names are editable labels and need not be unique.
Input symbols are individual Unicode scalar values (not grapheme clusters).
The empty string is accepted exactly when the initial configuration is
accepting.

- **DFA:** each state and input symbol has at most one destination. A missing
  transition rejects the input. Execution stops at that point and records the
  missing transition in its trace.
- **NFA:** a state and input symbol may have zero or more destinations. An
  epsilon transition consumes no input. Execution tracks a set of active
  states, not a single guessed path.
- **Alphabet:** the set of symbols used by non-epsilon transitions. For the
  first version it is derived from transitions, avoiding a second source of
  truth. The UI may display and edit this derived set by adding/removing
  transitions. An explicitly declared but unused alphabet can be considered
  later if needed.
- **Epsilon closure:** all states reachable from a set of states using zero or
  more epsilon transitions, including the original states.

NFA execution starts with the epsilon closure of the initial state. For each
input symbol, it collects every destination reachable from the current active
set by that symbol, then takes the epsilon closure of the collected set. The
input is accepted if the final active set intersects `F`. An empty active set
remains empty for the rest of the run.

## Architectural boundaries

```text
Qt Explorer
  ├── document/editor model and undoable commands
  ├── graph presentation (nodes, edges, labels)
  └── execution playback controls
          │
          ▼
libfsm (Qt-independent)
  ├── automaton interface and DFA/NFA implementations
  ├── canonical mathematical description
  ├── execution session and trace
  └── validated serialization codec / file format
```

The library must not include Qt headers or depend on Qt types. The GUI converts
between Qt events/graphics and library values at the application boundary.
Serialization should also be callable from tests or another client without
opening a GUI.

## Library design

Use value types for identifiers, symbols, canonical descriptions, and trace
records; use polymorphism where behavior genuinely differs.

### Public concepts

- `StateId`: stable identity for a state, distinct from its user-visible name.
- `Symbol`: one input symbol. Epsilon is represented as a distinct transition
  label and is not a member of `Σ`.
- `Transition`: source ID, label (input symbol or epsilon), and destination
  ID. Each transition also has a stable ID for editing and GUI selection.
- `Automaton`: common read/query interface, exposing its kind, initial state,
  state and accepting-state queries, `step`/run creation, and a canonical
  description.
- `Dfa` and `Nfa`: concrete polymorphic automata with type-specific
  transition rules and execution behavior.
- `ExecutionSession`: mutable state for one run over an immutable automaton.
  It advances one input symbol at a time and exposes the current configuration
  and trace.
- `AutomatonDescription`: canonical, storage-independent snapshot containing
  the kind, states, alphabet, initial state, accepting states, and transition
  set. This is the mathematical/display representation, not necessarily the
  internal storage.
- `AutomatonBuilder` (or equivalent validated editor API): creates and edits
  automata while enforcing invariants. Avoid exposing mutable internal
  containers to callers.

The exact namespace and file layout can be chosen when implementation starts.
Keep public headers small and make implementation details private.

### Internal representation

- Store states and accepting membership using stable IDs with efficient
  lookup. A state record can hold its ID and display name; acceptance should
  be a property of the automaton, not a special state subclass.
- A DFA stores a direct lookup from `(state, symbol)` to one destination,
  making a transition step a map lookup. Enforce at most one destination per
  pair when building or editing.
- An NFA stores outgoing destinations grouped by `(state, symbol)` plus a
  separate epsilon adjacency structure. A step unions destinations for every
  active state, then computes epsilon closure.
- Precompute or memoize epsilon closures only if profiling justifies it.
  Correctness must not depend on caching; edits invalidate any cache.
- Produce the canonical description from these structures on request. Keep
  returned ordering deterministic (for example, by stable ID and symbol) so
  serialized output, tests, and UI lists are reproducible.

### Validation and editing rules

An automaton is valid only if it has at least one state, its initial state
exists, every transition endpoint exists, and every accepting state exists.
A DFA must not contain multiple destinations for the same source and symbol.
An NFA may have any number of destinations, including zero, and may have
epsilon transitions. Reject malformed edits with an explicit error; do not
silently discard data or substitute a default state.

Whether an empty automaton should be representable as an unsaved editor
document is an application concern. It should not be executable or serializable
as a valid automaton until it has a state and initial state.

### Execution and trace model

Creating a session binds it to an automaton and an input string. Each advance
consumes one input symbol or reports a terminal condition. The session exposes:

- input and current input position;
- the active state set (a singleton for a running DFA);
- accepted, rejected, or still-running status;
- a trace entry for the initial configuration and each consumed symbol;
- for a failed DFA transition, the missing symbol and source state.

Trace data should describe observable configurations, not retain pointers into
automaton internals. For an NFA, the primary trace is the active set after each
symbol, including epsilon closure. Showing the individual transitions that
contributed to a step can be added as optional detail without changing the
acceptance algorithm. Epsilon moves do not consume input, so they should not
appear as extra input positions in the basic playback timeline.

Treat the automaton as immutable for the lifetime of an execution session.
The application should either stop/reset playback after an edit or keep a
snapshot of the version being executed; it must not mix trace data from
different automaton versions.

## Serialization

Use a versioned, human-readable JSON format for the first file format. Keep
encoding and decoding in a library module with APIs conceptually like
`encode(description)` and `decode(text) -> result`; the file dialog and file I/O
belong to the application. The codec must report parse, unsupported-version,
and validation errors explicitly.

The document stores:

- format identifier and schema version;
- automaton kind (`dfa` or `nfa`);
- state IDs, display names, and layout coordinates;
- initial state and accepting state IDs;
- transitions, including an explicit epsilon label for NFA transitions.

The alphabet is reconstructed from non-epsilon transitions. Layout is
presentation metadata and must not affect library semantics. Keep it separated
from the canonical mathematical description (for example, in an editor
document wrapper), even if both are encoded in one JSON file. Serialize
transitions deterministically. Loading must validate the complete document
before replacing the current editor document. Saving should avoid overwriting
the current file on a failed encode or write.

## Qt Explorer design

The application should have an editor document that owns the current
automaton, layout metadata, and dirty/file state. Graphics items render that
document; they are not the authoritative automaton storage. User operations
such as adding a state, changing acceptance, or adding a transition should go
through commands (for example, Qt undo commands) that update the document and
then refresh affected graphics.

The graph view needs state nodes, directed transitions, edge labels (with a
visibly distinct epsilon label), selection, drag-to-position, and creation or
editing interactions. Provide a clear way to select the initial state and
toggle accepting states. Prevent invalid DFA transitions or report the
validation error in the UI. Keep rendering and editing interactions separate
from the library's transition algorithms.

The run view accepts an input string, starts/resets execution, advances a
single symbol at a time, and highlights the configuration recorded by the
trace. It should show the final accepted/rejected result and make an early
missing DFA transition understandable. Continuous play can be layered on top
of single-step execution.

## Suggested repository organization

Names are suggestions; preserve the existing `libfsm` and `app` targets where
practical.

```text
libfsm/
  include/fsm/automaton.h
  include/fsm/dfa.h
  include/fsm/nfa.h
  include/fsm/description.h
  include/fsm/execution.h
  include/fsm/serialization.h
  src/...
  tests/...
app/
  src/main.cpp
  src/document/...
  src/graphics/...
  src/widgets/...
  tests/...
docs/
  design.md
```

## Work breakdown

Each item below is scoped to become an issue. The order captures real
dependencies, not calendar or sprint assignments.

### 1. Establish library build and test foundations

- Define the C++ standard and public include conventions.
- Add a test target and a small smoke test for the library.
- Keep the existing executable usable while the Qt application is introduced.
- Decide how warnings and test dependencies are configured.

**Done when:** the library and a minimal test executable build through the
project's normal CMake flow.

### 2. Define core value types and automaton invariants

- Add stable state and transition IDs, symbols, labels, state records, and
  canonical description types.
- Implement validation and explicit error results for invalid descriptions.
- Define deterministic ordering for canonical output.

**Done when:** descriptions can be constructed and invalid references or
malformed automata produce testable errors.

### 3. Implement DFA storage and execution

- Add the common automaton query interface and concrete DFA representation.
- Enforce one destination per `(state, symbol)`.
- Implement single-step execution, missing-transition rejection, and traces.

**Done when:** unit tests cover empty input, acceptance, rejection, and a
missing transition.

### 4. Implement NFA storage, epsilon closure, and execution

- Add NFA transitions with multiple destinations and epsilon adjacency.
- Implement epsilon closure with cycle-safe traversal.
- Implement active-set stepping and final acceptance by intersection with
  accepting states.
- Add tests for branching, epsilon cycles, empty active sets, and empty input.

**Done when:** tests verify the active set after each symbol and its acceptance
result, including epsilon-only paths.

### 5. Add canonical views and behavioral parity tests

- Expose alphabet, states, initial/accepting states, and the transition set
  through the canonical description for both implementations.
- Verify that canonical output faithfully represents each internal model.
- Keep output order stable across repeated calls.

**Done when:** consumers can display either automaton without depending on its
internal containers.

### 6. Add versioned serialization

- Define the JSON schema and format/version fields.
- Encode and decode canonical semantics and separate editor layout metadata.
- Validate a whole document on load and return useful, explicit errors.
- Add round-trip, malformed-file, unsupported-version, and determinism tests.

**Done when:** a valid saved document reloads with the same semantics and
layout, while invalid documents fail without partial results.

### 7. Create the Qt application shell and document model

- Add Qt dependencies and an application window.
- Create a document model that owns automaton and layout data.
- Connect open/save actions to the library codec and ordinary file I/O.
- Track dirty state and handle load/save errors visibly.

**Done when:** the app can open, save, and close a document independently of
the graph editing tools.

### 8. Build graph editing and undoable operations

- Render states and directed, labeled transitions.
- Add state creation, selection, moving, initial/accepting toggles, and
  transition creation/edit/removal.
- Route edits through validated document operations; enforce DFA constraints.
- Add undo/redo if using Qt's command stack for edits.

**Done when:** users can build a small valid DFA and NFA graphically, save it,
and reload it with the same graph layout and semantics.

### 9. Add input execution and visual playback

- Add input entry, reset, single-step, and final result controls.
- Render active-state highlighting from execution trace snapshots.
- Show the consumed input position and explain missing DFA transitions.
- Ensure editing cannot corrupt an active run's snapshot.

**Done when:** a user can inspect every step of DFA and NFA runs in the GUI and
see the same outcome as library tests.

### 10. Polish usability and documentation

- Add keyboard shortcuts, useful empty/error states, and basic accessibility
  labels.
- Document file format compatibility and basic application usage.
- Exercise representative end-to-end workflows manually and in UI tests where
  practical.

**Done when:** the editor, persistence, and playback workflows are coherent
enough for regular personal use.

## Testing strategy

- **Library unit tests:** invariants, DFA/NFA step behavior, epsilon closure,
  acceptance, trace positions, and canonical descriptions.
- **Serialization tests:** round trips, stable output, invalid references,
  duplicate DFA transitions, invalid JSON, and unsupported versions.
- **Application tests:** document commands and file operations separately
  from graphics where possible; add GUI tests for essential workflows when
  practical.
- **Cross-checks:** run the same small examples through the library and GUI,
  ensuring the displayed active configuration and final result match.

Prefer focused examples with explicitly stated expected active sets and traces;
accept/reject-only assertions can miss stepping and display errors.

## Design choices to revisit only when needed

- Whether to support alphabets larger than Unicode scalar values or tokenize
  input into multi-character symbols.
- Whether the format should preserve an explicitly declared alphabet containing
  symbols not currently used by transitions.
- Whether playback should optionally animate individual epsilon transitions or
  show only the mathematically standard active set after each input symbol.
- Whether determinization, minimization, or DFA totalization belongs in a later
  feature set.

These choices do not block the initial library, editor, file format, or
symbol-by-symbol execution.
