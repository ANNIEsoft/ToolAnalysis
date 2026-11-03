#include "ClusterTimeMerge.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <utility>

#include "Hit.h"

namespace ClusterTimeMerge {

double Median(std::vector<double> t) {
  if (t.empty()) return 0.;
  std::sort(t.begin(), t.end());
  size_t n = t.size();
  return (n % 2) ? t[n / 2] : 0.5 * (t[n / 2 - 1] + t[n / 2]);
}

template <typename HitT>
double MedianTime(const std::vector<HitT>& hits) {
  std::vector<double> t;
  t.reserve(hits.size());
  for (const auto& h : hits) t.push_back(h.GetTime());
  return Median(t);
}

template <typename HitT>
std::vector<std::vector<double>> Groups(const std::map<double, std::vector<HitT>>& clusters,
                                        double window, double min_time) {
  std::vector<std::pair<double, double>> order;  // (median, key)
  for (const auto& c : clusters) order.emplace_back(MedianTime(c.second), c.first);
  std::stable_sort(order.begin(), order.end(),
                   [](const std::pair<double, double>& a, const std::pair<double, double>& b) {
                     return a.first < b.first;
                   });
  std::vector<std::vector<double>> groups;
  bool have_prev = false;
  double prev = 0.;
  for (const auto& o : order) {
    if (o.first < min_time) {
      groups.push_back({o.second});
      continue;
    }
    if (!have_prev || o.first - prev >= window) groups.push_back({});
    groups.back().push_back(o.second);
    prev = o.first;
    have_prev = true;
  }
  return groups;
}

template <typename HitT>
int Merge(std::map<double, std::vector<HitT>>& clusters,
          std::map<double, std::vector<unsigned long>>& detkeys,
          double window, double min_time) {
  int removed = 0;
  for (const auto& group : Groups(clusters, window, min_time)) {
    if (group.size() < 2) continue;
    std::vector<HitT> hits;
    std::vector<unsigned long> keys;
    std::set<std::pair<int, double>> seen;
    for (double key : group) {
      const std::vector<HitT>& chits = clusters.at(key);
      const std::vector<unsigned long>& ckeys = detkeys.at(key);
      std::set<std::pair<int, double>> mine;
      for (size_t i = 0; i < chits.size(); ++i) {
        std::pair<int, double> id(chits[i].GetTubeId(), chits[i].GetTime());
        if (seen.count(id)) continue;
        mine.insert(id);
        hits.push_back(chits[i]);
        keys.push_back(ckeys.at(i));
      }
      seen.insert(mine.begin(), mine.end());
      clusters.erase(key);
      detkeys.erase(key);
    }
    double newkey = 0.;
    for (const auto& h : hits) newkey += h.GetTime();
    newkey /= hits.size();
    while (clusters.count(newkey)) newkey = std::nextafter(newkey, std::numeric_limits<double>::max());
    clusters.emplace(newkey, hits);
    detkeys.emplace(newkey, keys);
    removed += group.size() - 1;
  }
  return removed;
}

template double MedianTime<Hit>(const std::vector<Hit>&);
template double MedianTime<MCHit>(const std::vector<MCHit>&);
template std::vector<std::vector<double>> Groups<Hit>(const std::map<double, std::vector<Hit>>&, double, double);
template std::vector<std::vector<double>> Groups<MCHit>(const std::map<double, std::vector<MCHit>>&, double, double);
template int Merge<Hit>(std::map<double, std::vector<Hit>>&, std::map<double, std::vector<unsigned long>>&, double, double);
template int Merge<MCHit>(std::map<double, std::vector<MCHit>>&, std::map<double, std::vector<unsigned long>>&, double, double);

}  // namespace ClusterTimeMerge
