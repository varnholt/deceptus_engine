#pragma once

// the writer authors files into the working directory next to the source tree, which neither the
// console nor the web build has, and the console's romfs is read only on top of that. both are
// excluded explicitly because they can still be built with the development instrumentation for a
// profiling run.
#if defined(DEVELOPMENT_MODE) && !defined(__SWITCH__) && !defined(__EMSCRIPTEN__)
#define MECHANISM_SCHEMA_WRITER_ENABLED
#endif

#ifdef MECHANISM_SCHEMA_WRITER_ENABLED

void writeMechanismSchemas();

#endif  // MECHANISM_SCHEMA_WRITER_ENABLED
