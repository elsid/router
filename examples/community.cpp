#include <iostream>
#include <iterator>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include <router/router.hpp>

namespace model {

struct ConferenceId {
    std::string_view value;

    ConferenceId(std::string_view value) : value(value) {}
};

struct SpeakerId {
    std::string_view value;

    SpeakerId(std::string_view value) : value(value) {}
};

struct TalkId {
    std::string_view value;

    TalkId(std::string_view value) : value(value) {}
};

struct RoomId {
    std::string_view value;

    RoomId(std::string_view value) : value(value) {}
};

struct Speaker {};
struct Talk {};
struct Room {};

class Community {
public:
    std::optional<Speaker> get_speaker(ConferenceId conference_id, SpeakerId speaker_id) {
        std::cout << __func__ << " " << conference_id.value << " " << speaker_id.value << std::endl;
        return {};
    }

    Room add_room(ConferenceId conference_id) {
        std::cout << __func__ << " " << conference_id.value << std::endl;
        return {};
    }

    std::optional<Talk> remove_talk(ConferenceId conference_id, TalkId talk_id) {
        std::cout << __func__ << " " << conference_id.value << " " << talk_id.value << std::endl;
        return {};
    }

    std::vector<Talk> get_room_talks(ConferenceId conference_id, RoomId room_id) {
        std::cout << __func__ << " " << conference_id.value << " " << room_id.value << std::endl;
        return {};
    }

    std::vector<Speaker> get_room_speakers(ConferenceId conference_id, RoomId room_id) {
        std::cout << __func__ << " " << conference_id.value << " " << room_id.value << std::endl;
        return {};
    }
};

} // namespace model

namespace {

using model::Community;
using model::Room;
using model::Speaker;
using model::Talk;
using model::ConferenceId;
using model::SpeakerId;
using model::TalkId;
using model::RoomId;

using router::Selector;
using router::Action;

struct Tag {
    using value_type = std::string_view;
};

constexpr struct GetTag : Tag {
    static constexpr value_type value {"GET"};
} get_tag;

constexpr struct PostTag : Tag {
    static constexpr value_type value {"POST"};
} post_tag;

constexpr struct DeleteTag : Tag {
    static constexpr value_type value {"DELETE"};
} delete_tag;

constexpr struct ConferencesTag : Tag {
    static constexpr value_type value {"conferences"};
} conferences_tag;

constexpr struct SpeakersTag : Tag {
    static constexpr value_type value {"speakers"};
} speakers_tag;

constexpr struct RoomsTag : Tag {
    static constexpr value_type value {"rooms"};
} rooms_tag;

constexpr struct TalksTag : Tag {
    static constexpr value_type value {"talks"};
} talks_tag;

struct Serialize {
    void operator ()(std::monostate) const {}

    void operator ()(std::optional<Speaker>) const {}

    void operator ()(Room) const {}

    void operator ()(std::optional<Talk>) const {}

    void operator ()(std::vector<Talk>) const {}

    void operator ()(std::vector<Speaker>) const {}
};

struct Request {
    std::string method;
    std::vector<std::string> uri;
};

struct Iterator {
    using value_type = std::string_view;
    using difference_type = std::ptrdiff_t;

    const Request* request;
    std::size_t index = 0;

    std::string_view operator *() const {
        if (index < request->uri.size()) {
            return request->uri[index];
        }
        return request->method;
    }

    Iterator& operator++() {
        ++index;
        return *this;
    }

    Iterator operator++(int) {
        const Iterator result(*this);
        operator++();
        return result;
    }

    friend bool operator==(const Iterator& lhs, const Iterator& rhs) {
        return lhs.request == rhs.request && lhs.index == rhs.index;
    }
};

Iterator begin(const Request& request) {
    return Iterator {&request, 0};
}

Iterator end(const Request& request) {
    return Iterator {&request, request.uri.size() + 1};
}

constexpr Selector dispatch_impl(
    Action(conferences_tag, argument<ConferenceId>(
        Action(speakers_tag, argument<SpeakerId>(
            Action(get_tag, &Community::get_speaker)
        )),
        Action(talks_tag, argument<TalkId>(
            Action(delete_tag, &Community::remove_talk)
        )),
        Action(rooms_tag, Selector(
            Action(post_tag, &Community::add_room),
            argument<RoomId>(
                Action(talks_tag, Selector(Action(get_tag, &Community::get_room_talks))),
                Action(speakers_tag, Selector(Action(get_tag, &Community::get_room_speakers)))
            )
        ))
    ))
);

auto dispatch(Community& community, const Request& request) {
    return dispatch_impl(std::ranges::subrange(begin(request), end(request)), community);
}

} // namespace

int main() {
    model::Community community;
    Request request;
    try {
        request.method = "GET";
        request.uri = {"conferences", "cppnow2020", "speakers", "326"};
        std::visit(Serialize {}, dispatch(community, request));
        request.method = "POST";
        request.uri = {"conferences", "cppnow2020", "rooms"};
        std::visit(Serialize {}, dispatch(community, request));
        request.method = "DELETE";
        request.uri = {"conferences", "cppnow2020", "talks", "473"};
        std::visit(Serialize {}, dispatch(community, request));
        request.method = "GET";
        request.uri = {"conferences", "cppnow2020", "rooms", "3", "talks"};
        std::visit(Serialize {}, dispatch(community, request));
        request.method = "GET";
        request.uri = {"conferences", "cppnow2020", "rooms", "5", "speakers"};
        std::visit(Serialize {}, dispatch(community, request));
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return -1;
    }
    return 0;
}
