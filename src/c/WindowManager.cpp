#include "c/WindowManager.h"
#include "v/common/Renderer.h"

auto WindowManager::back() const -> const std::shared_ptr<Window> {
  if ( _population > 0 ) {
    return _windows.at(_population - 1);
  }
  return {};
}

Entity WindowManager::push( const std::shared_ptr<Window> win ) {
  std::unique_lock<std::mutex> l( _mut );  // This lets both timers and the controller trigger rendering.
  win->setBorder(true);
  if ( _population < _windows.size() ) {
    _windows.at(_population++) = win;
  }
  return win->getId();
}

Entity WindowManager::pop() {
  Entity nextWindowId{};
  std::unique_lock<std::mutex> l( _mut );  // This lets both timers and the controller trigger rendering.
  if ( _population > 0 ) {
    back()->clear();
    back()->setBorder(false);
    back()->repaint();
    _windows.at(--_population) = nullptr;
  }
  if ( _population > 0 ) {
    nextWindowId = back()->getId();
  }
  return nextWindowId;
}

void WindowManager::refreshAll() {
  auto& r = gfx::renderer();
  if ( r.checkResize() && !r.tooSmall() ) {
    for ( unsigned i = 0; i < _population; ++i ) {
      if ( _windows.at(i) != nullptr ) {
        _windows.at(i)->onTermResize();
      }
    }
  }
  if ( r.tooSmall() ) {  // windows keep their last good layout; they're re-fit once the screen grows back
    _pendingResize = true;
    r.endFrame();
    return;
  }
  if ( _pendingResize ) {
    _pendingResize = false;
    for ( unsigned i = 0; i < _population; ++i ) {
      if ( _windows.at(i) != nullptr ) {
        _windows.at(i)->onTermResize();
      }
    }
  }
  // clear, render, repaint. Pulled out so a 3D frame can hash that paint and, when it matches the framebuffer
  // already on the GPU, present without doing it again.
  auto paint = [&] {
    for ( unsigned i = 0; i < _population; ++i ) {
      if ( _windows.at(i) != nullptr ) {
        _windows.at(i)->clear();
        _windows.at(i)->render();
        _windows.at(i)->repaint();
      }
    }
  };
  // A coin changes one meter. Repaint that rectangle on the overlay already held, instead of the whole screen.
  if ( r.hudPatch() ) {
    r.beginHudPatch();
    paint();
    r.endFrame();
    if ( !r.takeOverlayRepaint() ) return;
  }
  if ( r.reuseOverlay() ) {
    r.setPixelWrites( false );
    paint();
    r.endFrame();
    if ( !r.takeOverlayRepaint() ) return;
  }
  paint();
  r.endFrame();  // displays results of the above window-painting
}

auto WindowManager::size() const -> int {
  return _population;
}

void WindowManager::render() {
  std::unique_lock<std::mutex> l( _mut );  // This lets both timers and the controller trigger rendering.
  refreshAll();  // clears, updates, and repaints each window prior to displaying
}

