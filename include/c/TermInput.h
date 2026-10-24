#pragma once
// Terminal (ASCII/SSH) input without a window system: decoding the byte stream and inferring key releases.
// Header-only and free of I/O so tests/term_input_test.cpp can drive it with synthetic bytes and timings.
//
// Terminals send bytes, not key events. Two sources of press/release:
//   1. The kitty keyboard protocol (kitty, foot, WezTerm, Ghostty, recent Alacritty/iTerm2...): asked for with
//      CSI > 11 u (disambiguate | report event types | report all keys as escape codes), it reports real presses,
//      repeats and releases (CSI code ; mods : event u). SGR mouse (?1006) reports button releases the same way.
//   2. Everywhere else (plain xterm, xterm's modifyOtherKeys, which has no release reports, tmux, Windows consoles)
//      a held key is its auto-repeat bytes, so KeyReleaseDetector presses on the first byte and releases it
//      `initial` ms later unless repeats follow: a tap is a tap (one step), whatever the typematic delay. When the
//      first repeat of a key just released that way arrives (a hold, after the typematic delay), the key is pressed
//      again and held while the repeats keep coming, until none comes within factor x the measured repeat interval
//      clamped to [min, max]. So a hold reads as press, release, (the terminal's delay), press ... release.
//      BICYCLE_TTY_KEYUP="initial,min,max,factor" tunes it (defaults "50,60,150,2.5").
#include "c/Input.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace term {

// A byte or code point to a logical key (letters, digits, space, enter, tab, escape, kitty's modifier codes).
inline auto keyOf( unsigned c ) -> LogicalKey {
  if ( c >= 'a' && c <= 'z' ) return static_cast<LogicalKey>( static_cast<int>( LogicalKey::A ) + ( c - 'a' ) );
  if ( c >= 'A' && c <= 'Z' ) return static_cast<LogicalKey>( static_cast<int>( LogicalKey::A ) + ( c - 'A' ) );
  if ( c >= '1' && c <= '9' ) return static_cast<LogicalKey>( static_cast<int>( LogicalKey::Key1 ) + ( c - '1' ) );
  switch ( c ) {
    case '0': return LogicalKey::Key0;
    case ' ': return LogicalKey::Space;
    case '\r': case '\n': return LogicalKey::Enter;
    case '\t': return LogicalKey::Tab;
    case 27: return LogicalKey::Escape;
    // kitty's functional key codes for the modifiers themselves
    case 57441: return LogicalKey::LeftShift;
    case 57442: return LogicalKey::LeftCtrl;
    case 57443: return LogicalKey::LeftAlt;
    case 57444: return LogicalKey::LeftMeta;
    case 57447: return LogicalKey::RightShift;
    case 57448: return LogicalKey::RightCtrl;
    case 57449: return LogicalKey::RightAlt;
    case 57450: return LogicalKey::RightMeta;
    default: return LogicalKey::COUNT;
  }
}

enum class Kind { BYTE, PRESS, REPEAT, RELEASE };  // BYTE: a legacy key byte, which says nothing about up or down
struct Event {
  LogicalKey key;
  Kind kind;
};

// Bytes in, key and mouse-button events out. Escape sequences (arrows, kitty keys, SGR mouse, the kitty query's
// reply) are parsed rather than read as letters. A lone ESC waits one frame for a sequence that may follow it.
class Decoder {
  public:
    int kittyFlags{ -1 };  // the terminal's answer to CSI ? u: >= 0 means the kitty protocol is on

    template <class F> void feed( unsigned char c, F&& out ) {
      if ( _buf.empty() ) {
        if ( c == 27 ) { _buf.push_back( static_cast<char>( c ) ); _age = 0; }
        else emit( out, keyOf( c ), Kind::BYTE );
        return;
      }
      _buf.push_back( static_cast<char>( c ) );
      if ( _buf.size() == 2 ) {
        if ( c == '[' || c == 'O' ) return;
        // ESC + key: Alt+key (or ESC typed just before a key): the key
        _buf.clear();
        if ( c == 27 ) { emit( out, LogicalKey::Escape, Kind::BYTE ); _buf.push_back( 27 ); _age = 0; }
        else emit( out, keyOf( c ), Kind::BYTE );
        return;
      }
      if ( c >= 0x40 && c <= 0x7e ) {  // a sequence's final byte
        parse( out );
        _buf.clear();
      }
      else if ( _buf.size() > 48 ) _buf.clear();  // garbage
    }
    // Once per frame after the bytes: a lone ESC from the previous frame is the Escape key.
    template <class F> void endFrame( F&& out ) {
      if ( _buf.empty() ) return;
      if ( ++_age < 2 ) return;
      if ( _buf.size() == 1 ) emit( out, LogicalKey::Escape, Kind::BYTE );
      _buf.clear();
    }

  private:
    std::string _buf;
    int _age{};
    template <class F> static void emit( F&& out, LogicalKey k, Kind kind ) {
      if ( k != LogicalKey::COUNT && k != LogicalKey::NOTHING ) out( Event{ k, kind } );
    }
    static auto kindOf( int ev ) -> Kind { return ev == 3 ? Kind::RELEASE : ev == 2 ? Kind::REPEAT : Kind::PRESS; }
    template <class F> void parse( F&& out ) {
      const char intro = _buf[1], fin = _buf.back();
      const std::string p = _buf.substr( 2, _buf.size() - 3 );
      if ( intro == 'O' ) {  // SS3 arrows (application cursor mode)
        arrow( out, fin, Kind::BYTE );
        return;
      }
      if ( !p.empty() && p[0] == '?' && fin == 'u' ) {  // reply to the kitty query: CSI ? flags u
        kittyFlags = std::atoi( p.c_str() + 1 );
        return;
      }
      if ( !p.empty() && p[0] == '<' && ( fin == 'M' || fin == 'm' ) ) {  // SGR mouse: CSI < b ; x ; y M|m
        const int b = std::atoi( p.c_str() + 1 );
        if ( b & ( 32 | 64 ) ) return;  // motion and wheel
        static constexpr LogicalKey BTN[] = { LogicalKey::MouseLeft, LogicalKey::MouseMiddle, LogicalKey::MouseRight, LogicalKey::COUNT };
        emit( out, BTN[b & 3], fin == 'M' ? Kind::PRESS : Kind::RELEASE );
        return;
      }
      // kitty: code[:alternates] [; mods[:event] [; text]]
      int code = std::atoi( p.c_str() ), event = 0;
      const auto semi = p.find( ';' );
      if ( semi != std::string::npos ) {
        const auto colon = p.find( ':', semi );
        const auto next = p.find( ';', semi + 1 );
        if ( colon != std::string::npos && ( next == std::string::npos || colon < next ) ) event = std::atoi( p.c_str() + colon + 1 );
      }
      if ( fin == 'u' ) emit( out, keyOf( static_cast<unsigned>( code ) ), kindOf( event ) );
      else if ( fin >= 'A' && fin <= 'D' ) arrow( out, fin, event ? kindOf( event ) : Kind::BYTE );
    }
    template <class F> static void arrow( F&& out, char fin, Kind kind ) {
      // Arrows drive the same keys as WASD (like SDL mode).
      const LogicalKey k = fin == 'A' ? LogicalKey::W : fin == 'B' ? LogicalKey::S : fin == 'C' ? LogicalKey::D
                         : fin == 'D' ? LogicalKey::A : LogicalKey::COUNT;
      emit( out, k, kind );
    }
};

// Press on a key's first byte, hold while auto-repeat bytes arrive, release when they stop. Times are in ms.
class KeyReleaseDetector {
  public:
    double initialMs{ 50 }, minMs{ 60 }, maxMs{ 150 }, factor{ 2.5 };
    double repeatMs{};  // measured auto-repeat interval (moving average), 0 = not measured yet
    double delayMs{};   // measured typematic delay (first byte to first repeat), 0 = not measured yet

    // "initial,min,max,factor"; any prefix may be given.
    void configure( const char* spec ) {
      if ( !spec ) return;
      double v[4] = { initialMs, minMs, maxMs, factor };
      std::sscanf( spec, "%lf,%lf,%lf,%lf", &v[0], &v[1], &v[2], &v[3] );
      initialMs = v[0]; minMs = v[1]; maxMs = std::max( v[1], v[2] ); factor = v[3];
    }
    // How long a held key may go without a byte before it counts as released.
    auto timeout( LogicalKey k ) const -> double {
      const auto& s = _k[idx( k )];
      if ( s.repeats == 0 ) return initialMs;  // a tap ends right away
      return repeatMs > 0 ? std::clamp( factor * repeatMs, minMs, maxMs ) : maxMs;
    }
    auto down( LogicalKey k ) const -> bool { return _k[idx( k )].down; }

    // A byte for key k at time t: true if it is a new press (not an auto-repeat of a held key).
    auto onByte( LogicalKey k, double t ) -> bool {
      auto& s = _k[idx( k )];
      if ( s.down && !s.real ) {
        const double gap = t - s.last;
        // Bytes are timestamped when read (once a frame), so a frame's repeats arrive as one burst: the interval is
        // the gap since the previous burst over the burst's size (summed up in expire()).
        if ( gap >= 5 ) {
          if ( s.repeats == 0 ) delayMs = avg( delayMs, t - s.pressT );
          else { s.pendGap = gap; s.pendN = 1; }
        }
        else if ( s.pendN > 0 ) ++s.pendN;
        ++s.repeats;
        s.last = t;
        return false;
      }
      // Released as a tap, then its first auto-repeat came (the terminal's typematic delay later): a hold. It's
      // pressed again and held like a repeating key. Another byte soon after the tap (a quick double tap) is a
      // fresh tap; once the delay is measured, only a byte near it counts as the repeat.
      const double since = t - _timedOutPressT;
      const bool lateRepeat = _timedOut == k && ( delayMs > 0 ? since > delayMs * 0.6 && since < delayMs * 1.6
                                                              : since >= 200 && since < 1200 );
      if ( lateRepeat ) delayMs = avg( delayMs, since );
      _timedOut = LogicalKey::COUNT;
      // (Terminals repeat only the newest key: a key held before it stops repeating and is released by its timeout.)
      s = Key{ true, false, t, t, lateRepeat ? 1 : 0 };
      return true;
    }
    // Real events (kitty protocol): these keys are released only by their release event.
    auto press( LogicalKey k, double t ) -> bool {
      auto& s = _k[idx( k )];
      const bool was = s.down;
      s = Key{ true, true, t, t, 0 };
      return !was;
    }
    auto release( LogicalKey k ) -> bool {
      auto& s = _k[idx( k )];
      const bool was = s.down;
      s.down = false;
      return was;
    }
    // Releases every byte-held key whose repeats stopped; calls released( key ) for each, in key order.
    template <class F> void expire( double t, F&& released ) {
      for ( unsigned i = 1; i < N; ++i ) {
        auto& s = _k[i];
        if ( s.pendN > 0 ) {
          repeatMs = avg( repeatMs, s.pendGap / s.pendN );
          s.pendN = 0;
        }
        if ( !s.down || s.real ) continue;
        const auto k = static_cast<LogicalKey>( i );
        if ( t - s.last <= timeout( k ) ) continue;
        if ( s.repeats == 0 ) { _timedOut = k; _timedOutPressT = s.pressT; }
        s.down = false;
        released( k );
      }
    }

  private:
    static constexpr unsigned N = static_cast<unsigned>( LogicalKey::COUNT );
    struct Key {
      bool down{}, real{};
      double pressT{}, last{};
      int repeats{};
      double pendGap{};  // this frame's repeat burst: gap since the previous one, and its size
      int pendN{};
    };
    std::array<Key, N> _k{};
    LogicalKey _timedOut{ LogicalKey::COUNT };
    double _timedOutPressT{};
    static auto idx( LogicalKey k ) -> unsigned { return std::min( static_cast<unsigned>( k ), N - 1 ); }
    static auto avg( double prev, double sample ) -> double { return prev > 0 ? prev * 0.7 + sample * 0.3 : sample; }
};

}  // namespace term
