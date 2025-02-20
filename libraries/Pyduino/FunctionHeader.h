#ifndef FUNCTIONHEADER_H
#define FUNCTIONHEADER_H

// Need to define in another header to avoid compilation errors
// Callback for custom messages
typedef long (*MessageCallback) (byte* message, int len, int type);

#endif
