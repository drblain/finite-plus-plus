#pragma once

#include <atomic>
#include <cstdint>
#include <string>

namespace fsm
{

template<typename ElementType>
struct ElementId
{
    uint64_t value;

    constexpr bool operator<=>(const ElementId&) const = default;

    explicit constexpr ElementId(uint64_t val): value(val) {}

    ElementId(): value(NextId()) {}

private:
    static uint64_t NextId()
    {
        static std::atomic<uint64_t> counter{0};
        return counter++;
    }
};

// forward declarations of State, Transition to define their id types
struct State;
struct Transition;
using StateId = ElementId<State>;
using TransitionId = ElementId<Transition>;

using Symbol = char8_t;

struct State
{
    StateId id;
    // std::string is temporary
    std::string displayName;
};

struct Transition
{
    TransitionId id;
    StateId source;
    Symbol label;
    StateId destination;
};

class Automaton
{
public:
    // virtual GetKind();
    // virtual GetInitialState();
    // virtual GetAcceptingState();
    // virtual GetCanonicalDescription();
    // virtual step??
};

}; // namespace fsm
