#pragma once

#define EXPORT_API \
  __attribute__((used))\
  __attribute__((visibility("default")))
