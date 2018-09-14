#include "generator/node_mixer.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

using namespace std;

namespace generator
{
void MixFakeNodes(istream & stream, function<void(OsmElement *)> processor)
{
  if (stream.fail())
    return;

  // Max node id on 12.02.2018 times hundred — good enough until ~2030.
  uint64_t constexpr baseNodeId = 5396734321 * 100;
  uint8_t constexpr kCFLat = 1;
  uint8_t constexpr kCFLon = 2;
  uint8_t constexpr kCFTags = 4;
  uint8_t constexpr kCFAll = kCFLat | kCFLon | kCFTags;
  uint64_t count = 0;
  uint8_t completionFlag = 0;
  OsmElement p;
  p.id = baseNodeId;
  p.type = OsmElement::EntityType::Node;

  string line;
  while (getline(stream, line))
  {
    if (line.empty())
    {
      if (completionFlag == kCFAll)
      {
        processor(&p);
        count++;
        p.Clear();
        p.id = baseNodeId + count;
        p.type = OsmElement::EntityType::Node;
        completionFlag = 0;
      }
      continue;
    }

    auto const eqPos = line.find('=');
    if (eqPos != string::npos)
    {
      string key = line.substr(0, eqPos);
      string value = line.substr(eqPos + 1);
      strings::Trim(key);
      strings::Trim(value);

      if (key == "lat")
      {
        if (strings::to_double(value, p.lat))
          completionFlag |= kCFLat;
      }
      else if (key == "lon")
      {
        if (strings::to_double(value, p.lon))
          completionFlag |= kCFLon;
      }
      else
      {
        p.AddTag(key, value);
        completionFlag |= kCFTags;
      }
    }
  }

  if (completionFlag == kCFAll)
  {
    processor(&p);
    count++;
  }

  LOG(LINFO, ("Added", count, "fake nodes."));
}
}  // namespace generator
