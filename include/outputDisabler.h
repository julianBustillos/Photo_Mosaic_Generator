#pragma once

#include <iostream>


#define START_DISABLE_STDERR FILE *stream; freopen_s(&stream, "nul", "w", stderr);
#define END_DISABLE_STDERR freopen_s(&stream, "CON", "w", stderr);
