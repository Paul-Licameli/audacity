/**********************************************************************

Audacity: A Digital Audio Editor

CommandID.h

Paul Licameli split from audacity/Types.h

**********************************************************************/

#ifndef __AUDACITY_COMMAND_ID__
#define __AUDACITY_COMMAND_ID__

#include "Identifier.h"

// Identifies a menu command or macro.
// Case-insensitive comparison
struct CommandIdTag;
using CommandID = TaggedIdentifier< CommandIdTag, false >;
using CommandIDs = std::vector<CommandID>;

using CommandParameter = CommandID;

#endif
