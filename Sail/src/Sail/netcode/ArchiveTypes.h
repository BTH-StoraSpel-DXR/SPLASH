#pragma once
#include "Sail/../../libraries/cereal/archives/portable_binary.hpp"

namespace Netcode {
	typedef cereal::PortableBinaryOutputArchive OutArchive; // Writes data to archive
	typedef cereal::PortableBinaryInputArchive  InArchive;  // Loads data from archive
}