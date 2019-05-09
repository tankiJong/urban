#pragma once

#include "engine/pch.h"

enum class eQueueType: unsigned;
class CommandQueue;

class Device {
public:
   S<CommandQueue> createCommandQueue(eQueueType type);
};
