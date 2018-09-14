#include "indexer/search_string_utils.hpp"
#include "indexer/string_set.hpp"

#include "base/assert.hpp"
#include "base/macros.hpp"
#include "base/mem_trie.hpp"

#include "3party/utfcpp/source/utf8/unchecked.h"

#include <algorithm>

using namespace std;
using namespace strings;

namespace search
{
namespace
{
// Replaces '#' followed by an end-of-string or a digit with space.
void RemoveNumeroSigns(UniString & s)
{
  size_t const n = s.size();

  size_t i = 0;
  while (i < n)
  {
    if (s[i] != '#')
    {
      ++i;
      continue;
    }

    size_t j = i + 1;
    while (j < n && IsASCIISpace(s[j]))
      ++j;

    if (j == n || IsASCIIDigit(s[j]))
      s[i] = ' ';

    i = j;
  }
}
}  // namespace

UniString NormalizeAndSimplifyString(string const & s)
{
  UniString uniString = MakeUniString(s);
  for (size_t i = 0; i < uniString.size(); ++i)
  {
    UniChar & c = uniString[i];
    switch (c)
    {
    // Replace "d with stroke" to simple d letter. Used in Vietnamese.
    // (unicode-compliant implementation leaves it unchanged)
    case 0x0110:
    case 0x0111:
      c = 'd';
      break;
    // Replace small turkish dotless 'ı' with dotted 'i'.  Our own
    // invented hack to avoid well-known Turkish I-letter bug.
    case 0x0131:
      c = 'i';
      break;
    // Replace capital turkish dotted 'İ' with dotted lowercased 'i'.
    // Here we need to handle this case manually too, because default
    // unicode-compliant implementation of MakeLowerCase converts 'İ'
    // to 'i' + 0x0307.
    case 0x0130:
      c = 'i';
      break;
    // Some Danish-specific hacks.
    case 0x00d8:  // Ø
    case 0x00f8:  // ø
      c = 'o';
      break;
    case 0x0152:  // Œ
    case 0x0153:  // œ
      c = 'o';
      uniString.insert(uniString.begin() + (i++) + 1, 'e');
      break;
    case 0x00c6:  // Æ
    case 0x00e6:  // æ
      c = 'a';
      uniString.insert(uniString.begin() + (i++) + 1, 'e');
      break;
    case 0x2116:  // №
      c = '#';
      break;
    }
  }

  MakeLowerCaseInplace(uniString);
  NormalizeInplace(uniString);

  // Remove accents that can appear after NFKD normalization.
  uniString.erase_if([](UniChar const & c) {
    // ̀  COMBINING GRAVE ACCENT
    // ́  COMBINING ACUTE ACCENT
    return (c == 0x0300 || c == 0x0301);
  });

  RemoveNumeroSigns(uniString);

  return uniString;

  /// @todo Restore this logic to distinguish и-й in future.
  /*
  // Just after lower casing is a correct place to avoid normalization for specific chars.
  static auto const isSpecificChar = [](UniChar c) -> bool
  {
    return c == 0x0439; // й
  };
  UniString result;
  result.reserve(uniString.size());
  for (auto i = uniString.begin(), end = uniString.end(); i != end;)
  {
    auto j = find_if(i, end, isSpecificChar);
    // We don't check if (j != i) because UniString and Normalize handle it correctly.
    UniString normString(i, j);
    NormalizeInplace(normString);
    result.insert(result.end(), normString.begin(), normString.end());
    if (j == end)
      break;
    result.push_back(*j);
    i = j + 1;
  }
  return result;
  */
}

UniString FeatureTypeToString(uint32_t type)
{
  string const s = "!type:" + to_string(type);
  return UniString(s.begin(), s.end());
}

namespace
{
char const * kStreetTokensSeparator = "\t -,.";

/// @todo Move prefixes, suffixes into separate file (autogenerated).
/// It's better to distinguish synonyms comparison according to language/region.
class StreetsSynonymsHolder
{
public:
  struct BooleanSum
  {
    using value_type = bool;

    BooleanSum() { Clear(); }

    void Add(bool value)
    {
      m_value = m_value || value;
      m_empty = false;
    }

    template <typename ToDo>
    void ForEach(ToDo && toDo) const
    {
      toDo(m_value);
    }

    void Clear()
    {
      m_value = false;
      m_empty = true;
    }

    bool Empty() const { return m_empty; }

    void Swap(BooleanSum & rhs)
    {
      swap(m_value, rhs.m_value);
      swap(m_empty, rhs.m_empty);
    }

    bool m_value;
    bool m_empty;
  };

  StreetsSynonymsHolder()
  {
    char const * affics[] =
    {
      // Russian
      "аллея", "бульвар", "набережная", "переулок", "площадь", "проезд", "проспект", "шоссе", "тупик", "улица", "тракт", "ал", "бул", "наб", "пер", "пл", "пр", "просп", "ш", "туп", "ул", "тр",

      // English
      "street", "avenue", "square", "road", "boulevard", "drive", "highway", "lane", "way", "circle", "st", "av", "ave", "sq", "rd", "blvd", "dr", "hwy", "ln",

      // Lithuanian
      "g", "pr", "pl", "kel",

      // Български език - Bulgarian
      "булевард", "бул", "площад", "пл", "улица", "ул", "квартал", "кв",

      // Canada - Canada
      "allee", "alley", "autoroute", "aut", "bypass", "byway", "carrefour", "carref", "chemin", "cercle", "circle", "côte", "crossing", "cross", "expressway", "freeway", "fwy", "line", "link", "loop", "parkway", "pky", "pkwy", "path", "pathway", "ptway", "route", "rue", "rte", "trail", "walk",

      // Cesky - Czech
      "ulice", "ul", "náměstí", "nám",

      // Deutsch - German
      "allee", "al", "brücke", "br", "chaussee", "gasse", "gr", "pfad", "straße", "str", "weg", "platz",

      // Español - Spanish
      "avenida", "avd", "avda", "bulevar", "bulev", "calle", "calleja", "cllja", "callejón", "callej", "cjon", "cllon", "callejuela", "cjla", "callizo", "cllzo", "calzada", "czada", "costera", "coste", "plza", "pza", "plazoleta", "pzta", "plazuela", "plzla", "tránsito", "trans", "transversal", "trval", "trasera", "tras", "travesía", "trva",

      // Français - French
      "rue", "avenue", "carré", "cercle", "route", "boulevard", "drive", "autoroute", "lane", "chemin",

      // Nederlands - Dutch
      "laan", "ln.", "straat", "steenweg", "stwg", "st",

      // Norsk - Norwegian
      "vei", "veien", "vn", "gaten", "gata", "gt", "plass", "plassen", "sving", "svingen", "sv",

      // Polski - Polish
      "aleja", "aleje", "aleji", "alejach", "aleją", "plac", "placu", "placem", "ulica", "ulicy",

      // Português - Portuguese
      "street", "avenida", "quadrado", "estrada", "boulevard", "carro", "auto-estrada", "lane", "caminho",

      // Română - Romanian
      "bul", "bdul", "blv", "bulevard", "bulevardu", "calea", "cal", "piața", "pţa", "pța", "strada", "stra", "stradela", "sdla", "stradă", "unitate", "autostradă", "lane",

      // Slovenščina - Slovenian
      "cesta",

      // Suomi - Finnish
      "kaari", "kri", "katu", "kuja", "kj", "kylä", "polku", "tie", "t", "tori", "väylä", "vlä",

      // Svenska - Swedish
      "väg", "vägen", "gatan", "gränd", "gränden", "stig", "stigen", "plats", "platsen",

      // Türkçe - Turkish
      "sokak", "sk", "sok", "sokağı", "cadde", "cd", "caddesi", "bulvar", "bulvarı",

      // Tiếng Việt – Vietnamese
      "quốc lộ", "ql", "tỉnh lộ", "tl", "Đại lộ", "Đl", "Đường", "Đ", "Đường sắt", "Đs", "Đường phố", "Đp", "vuông", "con Đường", "Đại lộ", "Đường cao tốc",

      // Українська - Ukrainian
      "дорога", "провулок", "площа", "шосе", "вулиця", "дор", "пров", "вул"
    };

    for (auto const * s : affics)
    {
      UniString const us = NormalizeAndSimplifyString(s);
      m_strings.Add(us, true /* end of string */);
    }
  }

  bool MatchPrefix(UniString const & s) const { return m_strings.HasPrefix(s); }
  bool FullMatch(UniString const & s) const { return m_strings.HasKey(s); }

private:
  base::MemTrie<UniString, BooleanSum, base::VectorMoves> m_strings;
};

StreetsSynonymsHolder g_streets;
}  // namespace

string DropLastToken(string const & str)
{
  search::Delimiters delims;
  using Iter = utf8::unchecked::iterator<string::const_iterator>;

  // Find start iterator of prefix in input query.
  Iter iter(str.end());
  while (iter.base() != str.begin())
  {
    Iter prev = iter;
    --prev;

    if (delims(*prev))
      break;

    iter = prev;
  }

  return string(str.begin(), iter.base());
}

UniString GetStreetNameAsKey(string const & name)
{
  if (name.empty())
    return UniString();

  UniString res;
  SimpleTokenizer iter(name, kStreetTokensSeparator);
  while (iter)
  {
    UniString const s = NormalizeAndSimplifyString(*iter);
    ++iter;

    res.append(s);
  }

  return (res.empty() ? NormalizeAndSimplifyString(name) : res);
}

bool IsStreetSynonym(UniString const & s)
{
  return g_streets.FullMatch(s);
}

bool IsStreetSynonymPrefix(UniString const & s)
{
  return g_streets.MatchPrefix(s);
}

bool ContainsNormalized(string const & str, string const & substr)
{
  UniString const ustr = NormalizeAndSimplifyString(str);
  UniString const usubstr = NormalizeAndSimplifyString(substr);
  return std::search(ustr.begin(), ustr.end(), usubstr.begin(), usubstr.end()) != ustr.end();
}

// StreetTokensFilter ------------------------------------------------------------------------------
void StreetTokensFilter::Put(strings::UniString const & token, bool isPrefix, size_t tag)
{
  if ((isPrefix && IsStreetSynonymPrefix(token)) || (!isPrefix && IsStreetSynonym(token)))
  {
    ++m_numSynonyms;
    if (m_numSynonyms == 1)
    {
      m_delayedToken = token;
      m_delayedTag = tag;
      return;
    }
    if (m_numSynonyms == 2)
      EmitToken(m_delayedToken, m_delayedTag);
  }
  EmitToken(token, tag);
}
}  // namespace search