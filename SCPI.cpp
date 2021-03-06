#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <vector>
#include <optional>
#include <cassert>
#include <functional>

using namespace std;

class Node {
public:
    struct MatchResult {
        const Node& leafNode;
        bool isQuery;
        std::string_view parameters;
    };

    using Result = std::optional<MatchResult>;
    using HandlerFunc = std::function<void(bool, std::string_view)>;

private:
    std::string_view _keyword;
    std::string_view _keywordShort;
    bool _optional;
    std::vector<Node> _children;
    HandlerFunc _handler;

public:
    void parse(std::string_view input) const {
        while(input.length() > 0) {
            if(input[0] == ';')
                input.remove_prefix(1);
            std::string_view::size_type semicolonIdx = std::min(input.length(), input.find(';'));
            match(input.substr(0, semicolonIdx));
            input.remove_prefix(semicolonIdx);
        }
    }

    Node(std::string_view keyword, bool optional, const std::vector<Node>& children, HandlerFunc handler = HandlerFunc())
        : _keyword(std::move(keyword))
        , _optional(optional)
        , _children(std::make_move_iterator(children.begin()), std::make_move_iterator(children.end()))
        , _handler(handler)
    {
        assert(!_keyword.empty());
        assert(std::isupper(*_keyword.begin()));
        std::string_view::iterator itEndShort = std::find_if(_keyword.begin(), _keyword.end(), [](auto c) { return std::islower(c); });
        _keywordShort = _keyword.substr(0, std::distance(_keyword.begin(), itEndShort));
    }

    Result matchChildren(std::string_view input) const
    {
        for (const auto& child : _children) {
            Result result = child.match(input);
            if (result) {
                return result;
            }
        }
        return std::nullopt;
    }

    Result match(std::string_view input) const
    {
        Result result = MatchResult{ *this, false };

        if (input.empty()) { return std::nullopt; }
        if (input[0] == ':') { input.remove_prefix(1); }

        std::string_view::size_type colonIdx = input.find(':');
        std::string_view::size_type spaceIdx = input.find(' ');

        colonIdx = std::min(input.length(), colonIdx);
        spaceIdx = std::min(input.length(), spaceIdx);

        std::string_view substring = input.substr(0, std::min(colonIdx, spaceIdx));
        std::string_view matchString = substring;

        if (*matchString.rbegin() == '?') {
            matchString.remove_suffix(1);
            result->isQuery = true;
        }

        auto caseInsensitiveCompare = [](auto l, auto r) {
            return std::tolower(l) == std::tolower(r);
        };

        if (!std::equal(matchString.begin(), matchString.end(), _keywordShort.begin(), _keywordShort.end(), caseInsensitiveCompare) &&
                !std::equal(matchString.begin(), matchString.end(), _keyword.begin(), _keyword.end(), caseInsensitiveCompare)) {
            result = std::nullopt;
        }

        if (result) {
            std::string_view subInput = input.substr(substring.length());
            if (!subInput.empty()) {
                if (subInput[0] == ':') {
                    return matchChildren(subInput);
                }
                else {
                    subInput.remove_prefix(1);
                    result->parameters = subInput;
                }
            }
            if (_handler) {
                _handler(result->isQuery, result->parameters);
            }
        }
        else if (_optional) {
            return matchChildren(input);
        }

        return result;
    }

    friend std::ostream& operator<<(std::ostream& stream, const Node& result);
};

std::ostream& operator<<(std::ostream& stream, const std::string_view& view) {
    return stream << std::string(view.data(), view.size());
}

std::ostream& operator<<(std::ostream& stream, const Node& node) {
    return stream << node._keyword;
}

std::ostream& operator<<(std::ostream& stream, const Node::Result& result) {
    stream << result->leafNode;
    if (result->isQuery) {
        stream << " (query)";
    }
    else if(!result->parameters.empty()) {
        stream << "(" << result->parameters << ")";
    }
    return stream;
}

void handleVoltage(bool isQuery, std::string_view parameter) {

    std::cout << "=> Handling voltage, isQuery: " << isQuery;
    if (!parameter.empty())
        std::cout << ", parameter: " << parameter;
    std::cout << std::endl;
}

void handleCurrent(bool isQuery, std::string_view parameter) {
    std::cout << "=> Handling current, isQuery: " << isQuery;
    if (!parameter.empty())
        std::cout << ", parameter: " << parameter;
    std::cout << std::endl;
}


void testParsing() {
    Node nodeMatcher("SENSor", false, {
                         Node("POWer", true, {
                             Node("CURrent", false, {}, &handleCurrent),
                             Node("VOLTage", false, {}, &handleVoltage)
                         })
                     });

    nodeMatcher.parse("sEnS:voltage 100V;sEnS:current 0.2ma");
}

bool test(std::string input) {

    Node nodeMatcher("SENSor", false, {
                         Node("POWer", true, {
                             Node("CURrent", false, {}),
                             Node("VOLTage", false, {}, &handleVoltage)
                         })
                     });

    Node::Result result = nodeMatcher.match(input);

    std::cout << "Input: \"" << input << "\"";
    if (result) {
        std::cout << " matched: " << result;
    }
    else {
        std::cout << " did not match.";
    }

    std::cout << std::endl;

    return false;
}

int main()
{
    testParsing();

    test("sEnS:currEnt");
    test("sEnS:voltage 100V");
    test("sEnS:voltage?");
    test("sEnSor:voltage");
    test("sEnSor:PoW:voltage");
    test("SENSOR:PoWer:voltage");

    return 0;
}
