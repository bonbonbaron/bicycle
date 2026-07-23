#include "v/Image.h"
#include "m/World.h"

// Determine the widest row.
Image::Image( const char* imgChars ) : string( imgChars ) {
  strToImg( imgChars );
}

Image::Image( const char* imgChars, Color color ) : string( imgChars ), color( color ) {
  strToImg( imgChars );
}

void Image::strToImg( const char* str ) {
  unsigned maxLineWidth{};
  bool continueLooking{ true };
  LineLimits currLineLims{};
  auto imgStr = std::string( str );
  while ( continueLooking ) {
    currLineLims.len = imgStr.find( "\n", currLineLims.start, 1 ) - currLineLims.start;
    // Stop looking when no newlines remain. Width of last line is from cursor to end of the string.
    if ( currLineLims.len < 0 ) {
      currLineLims.len = imgStr.size() - currLineLims.start;
      continueLooking = false;
    }
    // Add to line limits vector and prepare for next one.
    lineLimits.push_back( currLineLims );
    maxLineWidth = std::max( maxLineWidth, static_cast<unsigned>( currLineLims.len ) );
    currLineLims.start += currLineLims.len + 1;  // "+1" includes the newline character.
  }
  // Even though we're not right-padding the shorter lines with spaces, we'll treat
  // the grid as a rectangle for easier collision detection.
  // Now set the size you discovered above.
  size.w = maxLineWidth;
  size.h = lineLimits.size();
  size.d = 1;  // TODO check this later
}
