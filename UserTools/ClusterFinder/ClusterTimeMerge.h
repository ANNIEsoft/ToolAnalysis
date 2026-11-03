#ifndef ClusterTimeMerge_H
#define ClusterTimeMerge_H

#include <map>
#include <vector>

// Merging of clusters that are close in time, used by ClusterFinder (TimeMergeWindow).
//
// Clusters are ordered by the median time of their hits and chained: a cluster joins the
// current group when its median is less than `window` ns after the median of the cluster
// before it. Clusters whose median is earlier than `min_time` are never merged. The hits of a
// group are combined, skipping hits (same tube and time) already taken from an earlier cluster
// of the group; hits repeated inside one cluster are kept as ClusterFinder made them. The group
// is keyed by the mean time of its hits, the same time convention ClusterFinder uses.
// Available for Hit and MCHit (instantiated in ClusterTimeMerge.cpp).
namespace ClusterTimeMerge {

double Median(std::vector<double> t);

template <typename HitT>
double MedianTime(const std::vector<HitT>& hits);

// Cluster keys grouped for merging, groups and members in time order; singletons included.
template <typename HitT>
std::vector<std::vector<double>> Groups(const std::map<double, std::vector<HitT>>& clusters,
                                        double window, double min_time);

// Merges in place; returns the number of clusters removed.
template <typename HitT>
int Merge(std::map<double, std::vector<HitT>>& clusters,
          std::map<double, std::vector<unsigned long>>& detkeys,
          double window, double min_time);

}  // namespace ClusterTimeMerge

#endif
