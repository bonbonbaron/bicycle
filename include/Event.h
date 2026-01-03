#include <vector>
#include <memory>


class enum EventState { FAILED, IN_PROGRESS, SUCCESS };

class enum ConditionOp {
  GT, GTE, LT, LTE, EQ, NE, IS_SET, IS_UNSET, AND, OR
};

class Event {
  public: 
    Event();
    Event( const std::vector<Condition>& conditions );
    auto getState() const -> EventState;
    auto meetsAllConditions() const -> bool;  // THIS, my boy, allows games to be pure data.
    auto meetsAnyCondition() const -> bool;  // THIS, my boy, allows games to be pure data.

    /// Must be overridden.
    virtual void run( bool condition ) = 0;

  private:

    // How will I handle auto-generating pointers to game states? Like my gold count?
    // The game will have to hard-code that in somewhere. 
    // OR....
    // I generalize access to game states with a map and keys.
    // There can be various types:
    //    bitsets
    //    integers
    //    
    /* 
     * Nah dude, you ought to be able to store game states in structs too!
     * Like your heroes' stats.
     * That's a game state; is Cyan dead? What level is he on? etc.
     * Templatize it.
     *
     * Maybe that's too ambitious. It should have limited scope so we don't try to code 
     * the whole engine in a YAML file lol. I still like the idea of events though.
     * I'm just not sure how I'm going
     */

      struct Condition {
        ConditionOp op;
        T operand1;
        T operand2;
        auto isMet() -> bool;
      };



    EventState _state{};
    std::vector<Condition> _conditions{};
    // Some events will need a trigger. What kinds of triggers are there?
    //    a bit being set or unset
    //    a mathematical condition
    //    a logical condition
    //    an item being possessed
    //
    //    I can make an execute() function that consumes a boolean defaulted to true. 
    //    So when I code the game engine wrapper, I can specify each event tree node one time only (e.g. "Gain an item IF you have enough gold").
    //
    //    
    //  If I can generalize events, I won't have to come up with a different lambda for every single little thing in the game.
    //  But I ought to leave room for custom-made events.
    /* Types of events:
     * ================
     *  List available actions
     *  Battle
     *  Music
     *  Dialogue
     *  Narration
     *  Menu
     *  Sequence
     *  Custom (is there a way we can avoid specific coding for each custom event type?)
     */

    /* Eh, one more design doubt has entered my mind too:
     * Sequence ought to be a type of event. As well as selector and the other behavior node types.
     * I guess there's a behavior graph and a location graph. Event *tree*, I should say.
     */

    /* Oh, and another one!
     * If Node and Event can be extended to more specific types, then maybe they ought to be spirited away into the heart of bicycle as well.
     */
};
