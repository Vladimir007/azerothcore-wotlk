#ifndef CHAT_COMMAND_H
#define CHAT_COMMAND_H

#include <cstddef>
#include <map>
#include <tuple>
#include <variant>
#include <vector>

#include "ChatCommandArgs.h"
#include "ChatCommandTags.h"
#include "Errors.h"
#include "Language.h"
#include "Optional.h"
#include "StringFormat.h"
#include "Util.h"

class ChatHandler;

namespace Acore::ChatCommands
{
    enum class SuperuserOnly : bool
    {
        No = false,
        Yes = true
    };

    struct ChatCommandBuilder;
    using ChatCommandTable = std::vector<ChatCommandBuilder>;
}

namespace Acore::Impl::ChatCommands
{
    // Forward declaration.
    // ConsumeFromOffset contains the bounds check for offset, then hands off to MultiConsumer
    // the call stack is MultiConsumer -> ConsumeFromOffset -> MultiConsumer -> ConsumeFromOffset etc.
    // MultiConsumer goes into ArgInfo for parsing on each iteration
    template <typename Tuple, std::size_t offset>
    ChatCommandResult ConsumeFromOffset(Tuple&, ChatHandler const* handler, std::string_view args);

    template <typename Tuple, typename NextType, std::size_t offset>
    struct MultiConsumer
    {
        static ChatCommandResult TryConsumeTo(Tuple& tuple, ChatHandler const* handler, std::string_view args)
        {
            ChatCommandResult next = ArgInfo<NextType>::TryConsume(std::get<offset>(tuple), handler, args);
            if (next)
                return ConsumeFromOffset<Tuple, offset + 1>(tuple, handler, *next);
            return next;
        }
    };

    template <typename Tuple, typename NestedNextType, std::size_t offset>
    struct MultiConsumer<Tuple, Optional<NestedNextType>, offset>
    {
        static ChatCommandResult TryConsumeTo(Tuple& tuple, ChatHandler const* handler, std::string_view args)
        {
            // Try with the argument
            auto& myArg = std::get<offset>(tuple);
            myArg.emplace();

            ChatCommandResult result1 = ArgInfo<NestedNextType>::TryConsume(myArg.value(), handler, args);
            if (result1)
                if ((result1 = ConsumeFromOffset<Tuple, offset + 1>(tuple, handler, *result1)))
                    return result1;
            // Try again omitting the argument
            myArg = std::nullopt;
            ChatCommandResult result2 = ConsumeFromOffset<Tuple, offset + 1>(tuple, handler, args);
            if (result2)
                return result2;
            if (result1.HasErrorMessage() && result2.HasErrorMessage())
            {
                return Acore::StringFormat("{} \"{}\"\n{} \"{}\"",
                    GetNcoreString(handler, LANG_CMDPARSER_EITHER), result2.GetErrorMessage(),
                    GetNcoreString(handler, LANG_CMDPARSER_OR), result1.GetErrorMessage());
            }
            if (result1.HasErrorMessage())
                return result1;
            return result2;
        }
    };

    template <typename Tuple, std::size_t offset>
    ChatCommandResult ConsumeFromOffset([[maybe_unused]] Tuple& tuple, [[maybe_unused]] ChatHandler const* handler, std::string_view args)
    {
        if constexpr (offset < std::tuple_size_v<Tuple>)
            return MultiConsumer<Tuple, std::tuple_element_t<offset, Tuple>, offset>::TryConsumeTo(tuple, handler, args);
        else if (!args.empty()) /* the entire string must be consumed */
            return std::nullopt;
        else
            return args;
    }

    template <typename T> struct HandlerToTuple { static_assert(Acore::dependant_false_v<T>, "Invalid command handler signature"); };
    template <typename... Ts> struct HandlerToTuple<bool(ChatHandler*, Ts...)> { using type = std::tuple<ChatHandler*, std::remove_cvref_t<Ts>...>; };
    template <typename T> using TupleType = HandlerToTuple<T>::type;

    struct CommandInvoker
    {
        CommandInvoker() : _wrapper(nullptr), _handler(nullptr) {}

        template <typename TypedHandler>
        explicit CommandInvoker(TypedHandler& handler)
        {
            _wrapper = [](void* h, ChatHandler* chatHandler, std::string_view argsStr)
            {
                using Tuple = TupleType<TypedHandler>;

                Tuple arguments;
                std::get<0>(arguments) = chatHandler;
                const ChatCommandResult result = ConsumeFromOffset<Tuple, 1>(arguments, chatHandler, argsStr);
                if (result)
                    // ReSharper disable once CppReinterpretCastFromVoidPtr
                    return std::apply(reinterpret_cast<TypedHandler*>(h), std::move(arguments));
                if (result.HasErrorMessage())
                    SendErrorMessageToHandler(chatHandler, result.GetErrorMessage());
                return false;
            };
            _handler = reinterpret_cast<void*>(handler);
        }

        explicit CommandInvoker(bool(&handler)(ChatHandler*, char const*))
        {
            _wrapper = [](void* h, ChatHandler* chatHandler, const std::string_view argsStr)
            {
                const std::string argsStrCopy(argsStr);
                return reinterpret_cast<bool(*)(ChatHandler*, char const*)>(h)(chatHandler, argsStrCopy.c_str());
            };
            _handler = reinterpret_cast<void*>(handler);
        }

        explicit operator bool() const { return (_wrapper != nullptr); }
        bool operator()(ChatHandler* chatHandler, const std::string_view args) const
        {
            ASSERT(_wrapper && _handler);
            return _wrapper(_handler, chatHandler, args);
        }

    private:
        using wrapper_func = bool(void*, ChatHandler*, std::string_view);
        wrapper_func* _wrapper;
        void* _handler;
    };

    class ChatCommandNode
    {
    friend struct FilteredCommandListIterator;
    using ChatCommandBuilder = Acore::ChatCommands::ChatCommandBuilder;

    public:
        static void LoadCommandMap();
        static void InvalidateCommandMap();
        static bool TryExecuteCommand(ChatHandler& handler, std::string_view cmdStr);
        static void SendCommandHelpFor(ChatHandler& handler, std::string_view cmdStr);
        static std::vector<std::string> GetAutoCompletionsFor(const ChatHandler& handler, std::string_view cmdStr);

        ChatCommandNode(): _superuserOnly(Acore::ChatCommands::SuperuserOnly::No) { }

    private:
        static const std::map<std::string_view, ChatCommandNode, StringCompareLessI_T>& GetTopLevelMap();
        static void LoadCommandsIntoMap(ChatCommandNode* blank, std::map<std::string_view, ChatCommandNode, StringCompareLessI_T>& map, const Acore::ChatCommands::ChatCommandTable& commands);

        void LoadFromBuilder(const ChatCommandBuilder& builder);
        ChatCommandNode(ChatCommandNode&& other) = default;

        void ResolveNames(std::string name);
        void SendCommandHelp(ChatHandler& handler) const;

        bool IsVisible(const ChatHandler& who) const { return IsInvokerVisible(who) || HasVisibleSubCommands(who); }
        bool IsInvokerVisible(const ChatHandler& who) const;
        bool HasVisibleSubCommands(const ChatHandler& who) const;

        std::string _name;
        CommandInvoker _invoker;
        Acore::ChatCommands::SuperuserOnly _superuserOnly;
        std::variant<std::monostate, AcoreStrings, std::string> _help;
        std::map<std::string_view, ChatCommandNode, StringCompareLessI_T> _subCommands;
    };
}

namespace Acore::ChatCommands
{
    struct ChatCommandBuilder
    {
        friend class Impl::ChatCommands::ChatCommandNode;

        struct InvokerEntry
        {
            template <typename T>
            InvokerEntry(T& handler, const AcoreStrings help, const SuperuserOnly superuserOnly):
                _invoker{ handler }, _help{ help }, _superuserOnly{ superuserOnly } { }

            InvokerEntry(const InvokerEntry&) = default;
            InvokerEntry(InvokerEntry&&) = default;

            Impl::ChatCommands::CommandInvoker _invoker;
            AcoreStrings _help;
            SuperuserOnly _superuserOnly;

            auto operator*() const { return std::tie(_invoker, _help, _superuserOnly); }
        };

        using SubCommandEntry = std::reference_wrapper<std::vector<ChatCommandBuilder> const>;

        ChatCommandBuilder(ChatCommandBuilder&&) = default;
        ChatCommandBuilder(const ChatCommandBuilder&) = default;

        template <typename TypedHandler>
        ChatCommandBuilder(const char* name, TypedHandler& handler, AcoreStrings help, SuperuserOnly superuserOnly):
            _name{ ASSERT_NOTNULL(name) }, _data{ std::in_place_type<InvokerEntry>, handler, help, superuserOnly } { }

        template <typename TypedHandler>
        ChatCommandBuilder(const char* name, TypedHandler& handler, SuperuserOnly superuserOnly)
            : ChatCommandBuilder(name, handler, AcoreStrings(), superuserOnly) { }

        ChatCommandBuilder(const char* name, const std::vector<ChatCommandBuilder>& subCommands):
            _name{ ASSERT_NOTNULL(name) }, _data{ std::in_place_type<SubCommandEntry>, subCommands } { }

    private:
        std::string_view _name;
        std::variant<InvokerEntry, SubCommandEntry> _data;
    };

    void LoadCommandMap();
    void InvalidateCommandMap();
    bool TryExecuteCommand(ChatHandler& handler, std::string_view cmd);
    void SendCommandHelpFor(ChatHandler& handler, std::string_view cmd);
    std::vector<std::string> GetAutoCompletionsFor(const ChatHandler& handler, std::string_view cmd);
}

#endif
