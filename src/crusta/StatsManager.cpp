#include <crusta/StatsManager.h>

using namespace std;


namespace crusta {

StatsManager statsMan;

Timer    StatsManager::timers[5];
#if CRUSTA_RECORD_STATS
ofstream StatsManager::file("timings.txt");
#else
ofstream StatsManager::file;
#endif //CRUSTA_RECORD_STATS

int StatsManager::numTiles           = 0;
int StatsManager::numSegments        = 0;
int StatsManager::maxSegmentsPerTile = 0;
int StatsManager::numData            = 0;
int StatsManager::numDataUpdated     = 0;


StatsManager::
~StatsManager()
{
    file.close();
}

void StatsManager::
StatsManager::
newFrame()
{
#if CRUSTA_RECORD_STATS
    //dump times
    for (int i=0; i<numTimers; ++i)
    {
        timers[i].stop();
        file << timers[i].seconds() << ",";
        timers[i].reset();
    }
    //dump usage stats
    file << numTiles <<","<< numSegments <<","<< maxSegmentsPerTile <<","<<
            numData <<","<< numDataUpdated <<","<< endl;
    numTiles = numSegments = maxSegmentsPerTile = numData = numDataUpdated = 0;

    timers[0].resume();
#endif //CRUSTA_RECORD_STATS
}

void StatsManager::
start(Stat stat)
{
#if CRUSTA_RECORD_STATS
    timers[stat].resume();
#endif //CRUSTA_RECORD_STATS
}

void StatsManager::
stop(Stat stat)
{
#if CRUSTA_RECORD_STATS
    timers[stat].stop();
#endif //CRUSTA_RECORD_STATS
}

void StatsManager::
extractTileStats(const SurfaceApproximation& surface)
{
#if CRUSTA_RECORD_STATS
    typedef NodeData::ShapeCoverage Coverage;

    numTiles = static_cast<int>(surface.visibles.size());

    //go through all the nodes provided
    for (int i=0; i<numTiles; ++i))
    {
        const NodeData& node     = *surface.visible(i).node;
        const Coverage& coverage = node.lineCoverage;

        if (coverage.empty())
            continue;

        ++numData;

        for (Coverage::const_iterator lit=coverage.begin();
             lit!=coverage.end(); ++lit)
        {
            int segsInTile     = static_cast<int>(lit->second.size());
            numSegments       += segsInTile;
            maxSegmentsPerTile = std::max(maxSegmentsPerTile, segsInTile);
        }
    }
#endif //CRUSTA_RECORD_STATS
}

void StatsManager::
incrementDataUpdated()
{
#if CRUSTA_RECORD_STATS
    ++numDataUpdated;
#endif //CRUSTA_RECORD_STATS
}

} //namespace crusta
