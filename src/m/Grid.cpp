#include "m/Grid.h"

void Grid::addLayer( const Layer& layer ) {
  _layers.push_back( layer );
}

void Grid::addEntity( const Entity entity, const unsigned layerIdx ) {

}

void Grid::moveEntity( const Entity entity, const unsigned layerIdx ) {
  
}

auto Grid::getLayers() -> std::vector<Layer>& {
  return _layers;
}
