// Terminal input (include/c/TermInput.h): the escape-sequence decoder and the key-release heuristic, driven with
// synthetic bytes and timings (ms). ctest: term-input.
#include "c/TermInput.h"
#include <cstdio>
#include <string>
#include <vector>

static int failures = 0;
#define CHECK( c ) do { if ( !( c ) ) { std::printf( "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c ); ++failures; } } while ( 0 )

using term::Kind;
using K = LogicalKey;

// What a game sees: "+D" pressed, "-D" released, at frame times.
struct Sim {
  term::Decoder dec;
  term::KeyReleaseDetector det;
  std::vector<std::string> log;
  static auto name( K k ) -> std::string {
    const int i = static_cast<int>( k );
    if ( k >= K::A && k <= K::Z ) return std::string( 1, static_cast<char>( 'A' + i - 1 ) );
    switch ( k ) {
      case K::Escape: return "Esc"; case K::Space: return "Space"; case K::Enter: return "Enter";
      case K::MouseLeft: return "MouseL"; case K::MouseRight: return "MouseR"; case K::MouseMiddle: return "MouseM";
      case K::LeftShift: return "Shift"; case K::Key7: return "7";
      default: return "#" + std::to_string( i );
    }
  }
  void event( const term::Event& ev, double t ) {
    switch ( ev.kind ) {
      case Kind::BYTE: if ( det.onByte( ev.key, t ) ) log.push_back( "+" + name( ev.key ) ); break;
      case Kind::PRESS: case Kind::REPEAT: if ( det.press( ev.key, t ) ) log.push_back( "+" + name( ev.key ) ); break;
      case Kind::RELEASE: if ( det.release( ev.key ) ) log.push_back( "-" + name( ev.key ) ); break;
    }
  }
  // One frame at time t: these bytes arrived since the last one.
  void frame( double t, const std::string& bytes = "" ) {
    auto out = [&]( const term::Event& ev ) { event( ev, t ); };
    for ( unsigned char c : bytes ) dec.feed( c, out );
    dec.endFrame( out );
    det.expire( t, [&]( K k ) { log.push_back( "-" + name( k ) ); } );
  }
  auto str() const -> std::string {
    std::string s;
    for ( const auto& e : log ) s += ( s.empty() ? "" : " " ) + e;
    return s;
  }
};

// Bytes of one key held from t0 to t1 with the given typematic delay and repeat interval, run frame by frame
// (33 ms) until tEnd; returns the frame times at which the press and release were seen.
static void holdKey( Sim& s, char c, double t0, double t1, double delay, double interval, double tEnd,
                     double* pressAt = nullptr, double* releaseAt = nullptr ) {
  std::vector<double> bytes{ t0 };
  for ( double t = t0 + delay; t <= t1; t += interval ) bytes.push_back( t );
  size_t next = 0;
  for ( double t = t0; t <= tEnd; t += 33.3 ) {
    std::string in;
    while ( next < bytes.size() && bytes[next] <= t ) { in += c; ++next; }
    const size_t before = s.log.size();
    s.frame( t, in );
    for ( size_t i = before; i < s.log.size(); ++i ) {
      if ( s.log[i][0] == '+' && pressAt ) *pressAt = t;
      if ( s.log[i][0] == '-' && releaseAt ) *releaseAt = t;
    }
  }
}

int main() {
  {  // a tap: one byte, released right away (no repeat came), so a game steps once
    Sim s;
    double p = -1, r = -1;
    holdKey( s, 'd', 0, 0, 1e9, 30, 800, &p, &r );
    CHECK( s.str() == "+D -D" );
    CHECK( p == 0 );
    CHECK( r > 50 && r < 110 );
  }
  {  // two quick taps: two presses, each released right away
    Sim s;
    for ( double t = 0; t < 600; t += 33.3 ) s.frame( t, ( t == 0 || ( t > 140 && t < 170 ) ) ? "d" : "" );
    CHECK( s.str() == "+D -D +D -D" );
  }
  {  // held for 2 s at a 500 ms delay and 30 Hz repeat: a tap, then pressed again by the first repeat and held
     // until shortly after the repeats stop
    Sim s;
    double p = -1, r = -1;
    holdKey( s, 'w', 0, 2000, 500, 33, 2600, &p, &r );
    CHECK( s.str() == "+W -W +W -W" );
    CHECK( p >= 500 && p < 540 );
    CHECK( r > 2000 && r <= 2000 + 150 + 34 );
    CHECK( s.det.repeatMs > 25 && s.det.repeatMs < 45 );
    CHECK( s.det.timeout( K::W ) == 150 || s.det.repeatMs * 2.5 <= 150 );
  }
  {  // a fast 50 Hz repeat: the release timeout shrinks toward its 60 ms floor
    Sim s;
    double r = -1;
    holdKey( s, 'a', 0, 1500, 300, 20, 2000, nullptr, &r );
    CHECK( s.str() == "+A -A +A -A" );
    CHECK( r > 1500 && r < 1500 + 33 + 67 + 34 + 34 );  // read latency + timeout (~2.5 x 27 ms) + frame steps
  }
  {  // a slow typematic delay (700 ms) is learned; a slow (10 Hz, Linux console) repeat still holds
    Sim s;
    holdKey( s, 's', 0, 1500, 700, 100, 2000 );
    CHECK( s.str() == "+S -S +S -S" );
    CHECK( s.det.delayMs > 650 );
    s.log.clear();
    holdKey( s, 's', 3000, 4500, 700, 33, 5000 );
    CHECK( s.str() == "+S -S +S -S" );
  }
  {  // two keys: terminals repeat only the newest; the first, a tap here, is released before the second's press
    Sim s;
    s.frame( 0, "w" );
    for ( double t = 33; t < 600; t += 33 ) s.frame( t, t >= 100 ? "d" : "" );
    CHECK( s.str() == "+W -W +D" );
  }
  {  // configuration from BICYCLE_TTY_KEYUP
    term::KeyReleaseDetector d;
    d.configure( "400,50,120,3" );
    CHECK( d.initialMs == 400 && d.minMs == 50 && d.maxMs == 120 && d.factor == 3 );
    d.configure( "250" );
    CHECK( d.initialMs == 250 && d.maxMs == 120 );
  }
  {  // escape sequences: arrows are WASD (not "ESC [ A" = A), a lone ESC is Escape a frame later, '7' is 7
    Sim s;
    s.frame( 0, "\x1b[C" );
    s.frame( 33, "\x1b" );
    CHECK( s.str() == "+D" );
    s.frame( 66 );
    CHECK( s.str() == "+D +Esc -D" );  // the arrow was a tap
    s.frame( 99, "7" );
    CHECK( s.str() == "+D +Esc -D +7" );
  }
  {  // SGR mouse: presses and real releases; wheel and motion ignored
    Sim s;
    s.frame( 0, "\x1b[<0;10;5M" );
    s.frame( 33, "\x1b[<64;10;5M\x1b[<32;11;5M" );
    s.frame( 66, "\x1b[<0;11;5m\x1b[<2;3;3M" );
    s.frame( 2000, "\x1b[<2;3;3m" );
    CHECK( s.str() == "+MouseL -MouseL +MouseR -MouseR" );  // no timeouts on real events
  }
  {  // kitty keyboard protocol: the query reply, then real press / repeat / release
    Sim s;
    s.frame( 0, "\x1b[?11u" );
    CHECK( s.dec.kittyFlags == 11 );
    s.frame( 33, "\x1b[119u" );            // w press
    s.frame( 1500, "\x1b[119;1:2u" );      // repeat, long after any timeout: still held
    CHECK( s.str() == "+W" );
    s.frame( 1533, "\x1b[119;1:3u" );      // release
    s.frame( 1566, "\x1b[57441;2u\x1b[1;1:1C" );  // left shift press, right arrow press (kitty form)
    s.frame( 1600, "\x1b[57441;1:3u\x1b[1;1:3C" );
    CHECK( s.str() == "+W -W +Shift +D -Shift -D" );
  }
  {  // Windows console records (term::ConsoleKeys)
    using K = LogicalKey;
    std::string log;
    auto emit = [&]( K k, bool down ) { log += std::string( log.empty() ? "" : " " ) + ( down ? "+" : "-" ) + ( k == K::A ? "A" : "D" ); };
    // A real console: down, repeated downs while held, then the up; a release only on the up.
    term::ConsoleKeys real;
    real.record( K::A, true, 0, emit ); real.endFrame( 0, emit );
    for ( double t = 33; t < 1000; t += 33 ) { real.record( K::A, true, t, emit ); real.endFrame( t, emit ); }
    CHECK( log == "+A" );
    real.record( K::A, false, 1000, emit ); real.endFrame( 1000, emit );
    CHECK( log == "+A -A" && !real.paired() );
    // VS Code's terminal through ConPTY: each character is a down+up pair. The key must stay pressed for frames,
    // not be released in the frame it was pressed (the hero never moved), and go up once the repeats stop.
    log.clear();
    term::ConsoleKeys pairs;
    pairs.record( K::D, true, 0, emit ); pairs.record( K::D, false, 0, emit ); pairs.endFrame( 0, emit );
    CHECK( log == "+D" && pairs.paired() );
    double t = 17;
    for ( ; t < 500; t += 17 ) pairs.endFrame( t, emit );  // the typematic delay: a tap's release, then
    for ( double r = 500; r < 1500; r += 33 ) {             // the repeats of a hold
      for ( ; t < r; t += 17 ) pairs.endFrame( t, emit );
      pairs.record( K::D, true, t, emit ); pairs.record( K::D, false, t, emit ); pairs.endFrame( t, emit );
      t += 17;
    }
    CHECK( log == "+D -D +D" );
    for ( ; t < 2000; t += 17 ) pairs.endFrame( t, emit );
    CHECK( log == "+D -D +D -D" );
  }
  if ( failures ) { std::printf( "term-input: %d failure(s)\n", failures ); return 1; }
  std::printf( "term-input: all passed\n" );
  return 0;
}
