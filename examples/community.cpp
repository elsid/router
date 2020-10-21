#include <algorithm>
#include <cstdio>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <random>
#include <ranges>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <variant>
#include <vector>

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

namespace router {

template <class Tag, class F>
struct Action {
    static constexpr typename Tag::value_type name = Tag::value;

    F f;

    template <class T>
    constexpr explicit Action(Tag, T&& f) : f(std::forward<T>(f)) {}

    template <class ... Args>
    auto operator ()(Args&& ... args) const -> std::invoke_result_t<F, Args&& ...> {
        return std::invoke(f, std::forward<Args>(args) ...);
    }
};

template <class Tag, class T>
Action(Tag, T&& f) -> Action<Tag, std::decay_t<T>>;

template <class F>
struct ArgumentsNumber {
    static constexpr std::size_t value = ArgumentsNumber<decltype(&F::operator())>::value - 1;
};

template <class R, class ... Args>
struct ArgumentsNumber<R (*)(Args ...)>
    : std::integral_constant<std::size_t, sizeof ... (Args)> {};

template <class T, class R, class ... Args>
struct ArgumentsNumber<R (T::*)(Args ...)>
    : std::integral_constant<std::size_t, sizeof ... (Args) + 1> {};

template <class T, class R, class ... Args>
struct ArgumentsNumber<R (T::*)(Args ...) const>
    : std::integral_constant<std::size_t, sizeof ... (Args) + 1> {};

template <class Tag, class F>
struct ArgumentsNumber<Action<Tag, F>> : ArgumentsNumber<F> {};

template <class F>
constexpr std::size_t arguments_number_v = ArgumentsNumber<F>::value;

template <class F>
struct ReturnType {
    using type = typename ReturnType<decltype(&F::operator())>::type;
};

template <class T, class R, class ... Args>
struct ReturnType<R (T::*)(Args ...)> {
    using type = R;
};

template <class T, class R, class ... Args>
struct ReturnType<R (T::*)(Args ...) const> {
    using type = R;
};

template <class R, class ... Args>
struct ReturnType<R (*)(Args ...)> {
    using type = R;
};

template <class Tag, class F>
struct ReturnType<Action<Tag, F>> {
    using type = typename ReturnType<F>::type;
};

template <class F>
using return_type_t = typename ReturnType<F>::type;

template <class ... Ts>
struct Visited {};

template <class ... Ts>
struct IsVisited : std::false_type {};

template <class ... Ts>
constexpr bool is_visited_v = IsVisited<Ts ...>::value;

template <class T, class H, class ... Ts>
struct IsVisited<T, Visited<H, Ts ...>> : IsVisited<T, Visited<Ts ...>> {};

template <class T, class ... Ts>
struct IsVisited<T, Visited<T, Ts ...>> : std::true_type {};

template <class T>
struct IsVisited<T, Visited<>> : std::false_type {};

template <class ... Ts>
struct Distinct : Distinct<Visited<>, Ts ...> {};

template <class T, class ... Vs, class ... Ts>
struct Distinct<Visited<Vs ...>, T, Ts ...>
        : std::conditional_t<
            is_visited_v<T, Visited<Vs ...>>,
            Distinct<Visited<Vs ...>, Ts ...>,
            Distinct<Visited<T, Vs ...>, Ts ...>
        > {};

template <class ... Vs, class ... Qs, class ... Ts>
struct Distinct<Visited<Vs ...>, std::variant<Qs ...>, Ts ...>
        : Distinct<Visited<Vs ...>, Qs ..., Ts ...> {};

template <class ... Ts>
struct Distinct<Visited<Ts ...>> {
    using type = std::variant<Ts ...>;
};

template <class T>
struct Distinct<Visited<T>> {
    using type = T;
};

template <class ... Ts>
using distinct_t = typename Distinct<Ts ...>::type;

inline auto consume(std::ranges::input_range auto input) {
    return std::ranges::subrange(std::next(std::begin(input)), std::end(input));
}

template <class Action, std::ranges::input_range Range, class ... Args>
auto invoke(const Action& action, Range input, Args&& ... args) {
    if constexpr (std::is_invocable_v<Action, Range, Args&& ...>) {
        return action(input, std::forward<Args>(args) ...);
    } else if constexpr (sizeof ... (Args) >= arguments_number_v<Action>) {
        if (!std::empty(input)) {
            throw std::runtime_error("Too many arguments");
        }
        return action(std::forward<Args>(args) ...);
    } else {
        if (std::empty(input)) {
            throw std::runtime_error("Not enough input");
        }
        return invoke(action, consume(input), std::forward<Args>(args) ..., *std::begin(input));
    }
}

template <class, class = std::void_t<>>
struct HasName : std::false_type {};

template <class T>
struct HasName<T, std::void_t<decltype(T::name)>> : std::true_type {};

template <class T>
constexpr bool has_name_v = HasName<T>::value;

template <class ... Actions>
struct Selector {
    using return_type = distinct_t<return_type_t<Actions> ...>;

    const std::tuple<Actions ...> actions;

    template <class ... Ts>
    constexpr explicit Selector(Ts&& ... actions) : actions(std::forward<Ts>(actions) ...) {}

    template <class ... Args>
    return_type operator ()(std::ranges::input_range auto input, Args&& ... args) const {
        if (std::empty(input)) {
            throw std::runtime_error("Not enough input");
        }
        return find_action(input, [&] (std::ranges::input_range auto input, const auto& action) {
            return invoke(action, input, std::forward<Args>(args) ...);
        });
    }

    template <std::size_t i = 0, class F>
    return_type find_action(std::ranges::input_range auto input, F&& f) const {
        if constexpr (i >= std::tuple_size_v<decltype(actions)>) {
            throw std::runtime_error("Invalid action: " + std::string(*std::begin(input)));
        } else if constexpr (has_name_v<std::tuple_element_t<i, decltype(actions)>>) {
            if (std::get<i>(actions).name == *std::begin(input)) {
                return make_result(f(consume(input), std::get<i>(actions)));
            }
            return find_action<i + 1>(input, std::forward<F>(f));
        } else {
            return make_result(f(input, std::get<i>(actions)));
        }
    }

    template <class ... Ts>
    static auto make_result(std::variant<Ts ...>&& value) {
        return std::visit(
            [] (auto&& v) { return return_type(std::move(v)); },
            std::move(value)
        );
    }

    template <class T>
    static auto make_result(T&& value) {
        return return_type(std::forward<T>(value));
    }
};

template <class ... Actions>
Selector(Actions&& ...) -> Selector<std::decay_t<Actions> ...>;

template <class Tag, class ... Actions>
struct ReturnType<Selector<Tag, Actions ...>> {
    using type = typename Selector<Tag, Actions ...>::return_type;
};

template <class T, class ... Actions>
struct Argument {
    Selector<Actions ...> selector;

    template <class ... F>
    constexpr explicit Argument(F&& ... f) : selector(std::forward<F>(f) ...) {}

    template <class ... Args>
    auto operator ()(std::ranges::input_range auto input, Args&& ... args) const {
        if (std::empty(input)) {
            throw std::runtime_error("Not enough input");
        }
        return selector(consume(input), std::forward<Args>(args) ..., T {*std::begin(input)});
    }
};

template <class T, class ... Actions>
constexpr auto argument(Actions&& ... actions) {
    return Argument<T, std::decay_t<Actions> ...>(std::forward<Actions>(actions) ...);
}

template <class T, class ... Actions>
struct ArgumentsNumber<Argument<T, Actions ...>> : ArgumentsNumber<Selector<Actions ...>> {};

template <class T, class ... Actions>
struct ReturnType<Argument<T, Actions ...>> : ReturnType<Selector<Actions ...>> {};

} // namespace router

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
