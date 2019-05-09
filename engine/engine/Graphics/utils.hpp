#pragma once

#include "engine/pch.h"

enum class eCommandType: uint {
   Empty = 0u,
   Copy = BIT_FLAG( 0 ),
   Compute = BIT_FLAG( 1 ),
   Graphics = BIT_FLAG( 2 ),
};
enum_class_operators( eCommandType );

enum class eQueueType: uint {
   Copy = 0u,
   Compute,
   Direct,
};
enum_class_operators( eQueueType );
