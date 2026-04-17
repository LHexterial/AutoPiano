#pragma once
#include "types.h"
#include <vector>
#include <string>

std::vector<Action> decodeMidi(const std::string &filePath, double startTime = 0.0);