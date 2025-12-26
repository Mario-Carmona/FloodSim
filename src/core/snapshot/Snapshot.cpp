
#include "core/snapshot/Snapshot.hpp"

namespace danasim {

	Snapshot::Snapshot()
		: step_(0)
		, time_(0.0f)
		, grid_(nullptr)
	{
	}

	Snapshot::Snapshot(uint64_t step, float time, const MapGrid* grid)
		: step_(step)
		, time_(time)
		, grid_(grid)
	{
	}

} // namespace danasim
