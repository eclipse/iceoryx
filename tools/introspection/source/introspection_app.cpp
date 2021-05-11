// Copyright (c) 2019 - 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2020 - 2021 by Apex.AI Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "iceoryx_introspection/introspection_app.hpp"
#include "iceoryx_introspection/introspection_types.hpp"
#include "iceoryx_posh/iceoryx_posh_types.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx_utils/internal/units/duration.hpp"
#include "iceoryx_versions.hpp"

#include <chrono>
#include <iomanip>
#include <poll.h>
#include <thread>

using namespace iox::client::introspection;
using namespace iox::units::duration_literals;

namespace iox
{
namespace client
{
namespace introspection
{
IntrospectionApp::IntrospectionApp(int argc, char* argv[]) noexcept
{
    if (argc < 2)
    {
        printShortInfo(argv[0]);
        exit(EXIT_FAILURE);
    }

    parseCmdLineArguments(argc, argv);
}

void IntrospectionApp::printHelp() noexcept
{
    std::cout << "Usage:\n"
                 "  introspection [OPTIONS] [SUBSCRIPTION]\n"
                 "  introspection --help\n"
                 "  introspection --version\n"
                 "\nOptions:\n"
                 "  -h, --help        Display help and exit.\n"
                 "  -t, --time <ms>   Update period (in milliseconds) for the display of introspection data\n"
                 "                    [min: "
              << MIN_UPDATE_PERIOD.toMilliseconds() << ", max: " << MAX_UPDATE_PERIOD.toMilliseconds()
              << ", default: " << DEFAULT_UPDATE_PERIOD.toMilliseconds()
              << "]\n"
                 "  -v, --version     Display latest official iceoryx release version and exit.\n"
                 "\nSubscription:\n"
                 "  Select which introspection data you would like to receive.\n"
                 "  --all             Subscribe to all available introspection data.\n"
                 "  --mempool         Subscribe to mempool introspection data.\n"
                 "  --port            Subscribe to port introspection data.\n"
                 "  --process         Subscribe to process introspection data.\n"
              << std::endl;
}

void IntrospectionApp::printShortInfo(const std::string& binaryName) noexcept
{
    std::cout << "Run '" << binaryName << " --help' for more information." << std::endl;
}

void IntrospectionApp::parseCmdLineArguments(int argc,
                                             char** argv,
                                             CmdLineArgumentParsingMode /*cmdLineParsingMode*/) noexcept
{
    int32_t opt;
    int32_t index;

    while ((opt = getopt_long(argc, argv, shortOptions, longOptions, &index)) != -1)
    {
        switch (opt)
        {
        case 'h':
            printHelp();
            exit(EXIT_SUCCESS);
            break;

        case 'v':
            std::cout << "Latest official iceoryx release version: " << ICEORYX_LATEST_RELEASE_VERSION << "\n"
                      << std::endl;
            exit(EXIT_SUCCESS);
            break;

        case 't':
        {
            uint64_t newUpdatePeriodMs;
            if (cxx::convert::fromString(optarg, newUpdatePeriodMs))
            {
                iox::units::Duration rate = iox::units::Duration::fromMilliseconds(newUpdatePeriodMs);
                updatePeriodMs = bounded(rate, MIN_UPDATE_PERIOD, MAX_UPDATE_PERIOD);
            }
            else
            {
                std::cout << "Invalid argument for `t`! Will be ignored!";
            }
            break;
        }

        case 0:
            if (longOptions[index].flag != 0)
                break;

            if (strcmp(longOptions[index].name, "all") == 0)
            {
                introspectionSelection.mempool = introspectionSelection.port = introspectionSelection.process = true;
                doIntrospection = true;
            }
            else if (strcmp(longOptions[index].name, "port") == 0)
            {
                introspectionSelection.port = true;
                doIntrospection = true;
            }
            else if (strcmp(longOptions[index].name, "process") == 0)
            {
                introspectionSelection.process = true;
                doIntrospection = true;
            }
            else if (strcmp(longOptions[index].name, "mempool") == 0)
            {
                introspectionSelection.mempool = true;
                doIntrospection = true;
            }

            break;

        case '?':
        default:
            printShortInfo(argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    if (!doIntrospection)
    {
        std::cout << "Wrong usage. ";
        printShortInfo(argv[0]);
        exit(EXIT_FAILURE);
    }
}

void IntrospectionApp::initTerminal()
{
    // Set up ncurses terminal
    initscr();
    curs_set(0);
    start_color();
    init_pair(static_cast<uint8_t>(ColorPairs::redOnBlack), COLOR_RED, COLOR_BLACK);
    init_pair(static_cast<uint8_t>(ColorPairs::whiteOnRed), COLOR_WHITE, COLOR_RED);

    // The pad should be big enough to hold all introspection data
    constexpr uint32_t padLines = 10000u; // we support up to 3000 ports, so this must be quite high
    constexpr uint32_t padCols = 1000u;
    pad = newpad(padLines, padCols);

    keypad(pad, TRUE);
    nodelay(pad, TRUE);
}

void IntrospectionApp::clearToBottom()
{
    wclrtobot(pad);
}

void IntrospectionApp::closeTerminal()
{
    endwin();
}

void IntrospectionApp::refreshTerminal()
{
    prefresh(pad, yPad, xPad, 0, 0, LINES - 1, COLS - 1);

    constexpr int32_t titleLines{0};
    wmove(pad, titleLines, 0);
}

void IntrospectionApp::updateDisplayYX()
{
    constexpr int32_t yIncrement = 1;
    constexpr int32_t xIncrement = 5;

    int32_t yMax = getmaxy(pad) - LINES;
    int32_t xMax = getmaxx(pad) - COLS;

    int32_t ch = wgetch(pad);
    if ((ch == KEY_UP) && (yPad > 0))
    {
        yPad -= yIncrement;
    }
    else if ((ch == KEY_DOWN) && ((yPad < yMax)))
    {
        yPad += yIncrement;
    }
    else if ((ch == KEY_LEFT) && (xPad > 0))
    {
        xPad -= xIncrement;
    }
    else if ((ch == KEY_RIGHT) && (xPad < xMax))
    {
        xPad += xIncrement;
    }
    else
    {
        // Nothing to do
    }
}

void IntrospectionApp::waitForUserInput(int32_t timeoutMs)
{
    struct pollfd fileDesc;
    fileDesc.fd = STDIN_FILENO;
    fileDesc.events = POLLIN;
    constexpr size_t nFileDesc = 1u;
    /// @todo Wrap kernel calls with SmartC
    int32_t eventCount = poll(&fileDesc, nFileDesc, timeoutMs);

    // Event detected
    if ((eventCount == nFileDesc) && (fileDesc.revents == POLLIN))
    {
        updateDisplayYX();
        refreshTerminal();
    }
}

void IntrospectionApp::prettyPrint(const std::string& str, const PrettyOptions pr)
{
    wattron(pad, prettyMap.find(pr)->second);
    wprintw(pad, str.c_str());
    wattroff(pad, prettyMap.find(pr)->second);
}

void IntrospectionApp::printProcessIntrospectionData(const ProcessIntrospectionFieldTopic* processIntrospectionField)
{
    constexpr int32_t pidWidth{-10};
    constexpr int32_t processWidth{-10};

    for (auto& data : processIntrospectionField->m_processList)
    {
        wprintw(pad, "PID: %*d Process: %*s\n", pidWidth, data.m_pid, processWidth, data.m_name.c_str());
    }
    wprintw(pad, "\n");
}

void IntrospectionApp::printMemPoolInfo(const MemPoolIntrospectionInfo& introspectionInfo)
{
    wprintw(pad, "Segment ID: %d\n", introspectionInfo.m_id);

    wprintw(pad, "Shared memory segment writer group: ");
    prettyPrint(std::string(introspectionInfo.m_writerGroupName), PrettyOptions::bold);
    wprintw(pad, "\n");

    wprintw(pad, "Shared memory segment reader group: ");
    prettyPrint(std::string(introspectionInfo.m_readerGroupName), PrettyOptions::bold);
    wprintw(pad, "\n\n");

    constexpr int32_t memPoolWidth{8};
    constexpr int32_t usedchunksWidth{14};
    constexpr int32_t numchunksWidth{9};
    constexpr int32_t minFreechunksWidth{9};
    constexpr int32_t chunkSizeWidth{11};
    constexpr int32_t chunkPayloadSizeWidth{13};

    wprintw(pad, "%*s |", memPoolWidth, "MemPool");
    wprintw(pad, "%*s |", usedchunksWidth, "Chunks In Use");
    wprintw(pad, "%*s |", numchunksWidth, "Total");
    wprintw(pad, "%*s |", minFreechunksWidth, "Min Free");
    wprintw(pad, "%*s |", chunkSizeWidth, "Chunk Size");
    wprintw(pad, "%*s\n", chunkPayloadSizeWidth, "Chunk Payload Size");
    wprintw(pad, "--------------------------------------------------------------------------------\n");

    for (size_t i = 0u; i < introspectionInfo.m_mempoolInfo.size(); ++i)
    {
        auto& info = introspectionInfo.m_mempoolInfo[i];
        if (info.m_numChunks > 0u)
        {
            wprintw(pad, "%*d |", memPoolWidth, i + 1u);
            wprintw(pad, "%*d |", usedchunksWidth, info.m_usedChunks);
            wprintw(pad, "%*d |", numchunksWidth, info.m_numChunks);
            wprintw(pad, "%*d |", minFreechunksWidth, info.m_minFreeChunks);
            wprintw(pad, "%*d |", chunkSizeWidth, info.m_chunkSize);
            wprintw(pad, "%*d\n", chunkPayloadSizeWidth, info.m_chunkPayloadSize);
        }
    }
    wprintw(pad, "\n");
}

void IntrospectionApp::printPortIntrospectionData(const std::vector<ComposedPublisherPortData>& publisherPortData,
                                                  const std::vector<ComposedSubscriberPortData>& subscriberPortData)
{
    constexpr int32_t serviceWidth{16};
    constexpr int32_t instanceWidth{16};
    constexpr int32_t eventWidth{21};
    constexpr int32_t runtimeNameWidth{23};
    constexpr int32_t nodeNameWidth{23};
    // uncomment once this information is needed
    // constexpr int32_t sampleSizeWidth{12};
    // constexpr int32_t chunkSizeWidth{12};
    // constexpr int32_t chunksWidth{12};
    // constexpr int32_t intervalWidth{19};
    constexpr int32_t subscriptionStateWidth{14};
    // constexpr int32_t fifoWidth{17};    // uncomment once this information is needed
    constexpr int32_t scopeWidth{12};
    constexpr int32_t interfaceSourceWidth{8};

    prettyPrint("Publisher Ports\n", PrettyOptions::bold);

    wprintw(pad, " %*s |", serviceWidth, "Service");
    wprintw(pad, " %*s |", instanceWidth, "Instance");
    wprintw(pad, " %*s |", eventWidth, "Event");
    wprintw(pad, " %*s |", runtimeNameWidth, "Process");
    wprintw(pad, " %*s |", nodeNameWidth, "Node");
    // uncomment once this information is needed
    // wprintw(pad, " %*s |", sampleSizeWidth, "Sample Size");
    // wprintw(pad, " %*s |", chunkSizeWidth, "Chunk Size");
    // wprintw(pad, " %*s |", chunksWidth, "Chunks");
    // wprintw(pad, " %*s |", intervalWidth, "Last Send Interval");
    wprintw(pad, " %*s\n", interfaceSourceWidth, "Src. Itf.");

    wprintw(pad, " %*s |", serviceWidth, "");
    wprintw(pad, " %*s |", instanceWidth, "");
    wprintw(pad, " %*s |", eventWidth, "");
    wprintw(pad, " %*s |", runtimeNameWidth, "");
    wprintw(pad, " %*s |", nodeNameWidth, "");
    // uncomment once this information is needed
    // wprintw(pad, " %*s |", sampleSizeWidth, "[Byte]");
    // wprintw(pad, " %*s |", chunkSizeWidth, "[Byte]");
    // wprintw(pad, " %*s |", chunksWidth, "[/Minute]");
    // wprintw(pad, " %*s |", intervalWidth, "[Milliseconds]");
    wprintw(pad, " %*s\n", interfaceSourceWidth, "");

    wprintw(pad, "---------------------------------------------------------------------------------------------------");
    wprintw(pad, "--------------------------------\n");

    bool needsLineBreak{false};
    uint32_t currentLine{0U};
    auto printEntry = [&](int32_t maxSize, const std::string& data) -> std::string {
        std::stringstream stream;

        constexpr int32_t indentation{2};
        constexpr char indentationString[indentation + 1] = "  ";

        auto stringSize = data.size();
        if (currentLine == 0U)
        {
            stream << std::left << std::setw(maxSize) << data.substr(0U, static_cast<size_t>(maxSize));
        }
        else if (stringSize
                 > static_cast<size_t>(maxSize) + (currentLine - 1U) * static_cast<size_t>(maxSize - indentation))
        {
            const auto startPosition =
                static_cast<size_t>(maxSize) + (currentLine - 1U) * static_cast<size_t>(maxSize - indentation);

            stream << indentationString << std::left << std::setw(maxSize - indentation)
                   << data.substr(startPosition, static_cast<size_t>(maxSize - indentation));
        }
        else
        {
            stream << std::left << std::setw(maxSize) << "";
        }

        needsLineBreak |=
            (stringSize > static_cast<size_t>(maxSize) + (currentLine) * static_cast<size_t>(maxSize - indentation));

        return stream.str();
    };

    for (auto& publisherPort : publisherPortData)
    {
        // uncomment once this information is needed
        // std::string m_sampleSize{"n/a"};
        // std::string m_chunkSize{"n/a"};
        // std::string m_chunksPerMinute{"n/a"};
        // std::string sendInterval{"n/a"};

        currentLine = 0;
        do
        {
            needsLineBreak = false;
            wprintw(pad, " %s |", printEntry(serviceWidth, publisherPort.portData->m_caproServiceID).c_str());
            wprintw(pad, " %s |", printEntry(instanceWidth, publisherPort.portData->m_caproInstanceID).c_str());
            wprintw(pad, " %s |", printEntry(eventWidth, publisherPort.portData->m_caproEventMethodID).c_str());
            wprintw(pad, " %s |", printEntry(runtimeNameWidth, publisherPort.portData->m_name).c_str());
            wprintw(pad, " %s |", printEntry(nodeNameWidth, publisherPort.portData->m_node).c_str());
            // uncomment once this information is needed
            // wprintw(pad, " %s |", printEntry(sampleSizeWidth, m_sampleSize).c_str());
            // wprintw(pad, " %s |", printEntry(chunkSizeWidth, m_chunkSize).c_str());
            // wprintw(pad, " %s |", printEntry(chunksWidth, m_chunksPerMinute).c_str());
            // wprintw(pad, " %s |", printEntry(intervalWidth, sendInterval).c_str());
            wprintw(
                pad,
                " %s\n",
                printEntry(interfaceSourceWidth,
                           (iox::capro::INTERFACE_NAMES[static_cast<std::underlying_type<iox::capro::Interfaces>::type>(
                               publisherPort.portData->m_sourceInterface)]))
                    .c_str());


            currentLine++;
        } while (needsLineBreak);
    }
    wprintw(pad, "\n");

    prettyPrint("Subscriber Ports\n", PrettyOptions::bold);

    wprintw(pad, " %*s |", serviceWidth, "Service");
    wprintw(pad, " %*s |", instanceWidth, "Instance");
    wprintw(pad, " %*s |", eventWidth, "Event");
    wprintw(pad, " %*s |", runtimeNameWidth, "Process");
    wprintw(pad, " %*s |", nodeNameWidth, "Node");
    wprintw(pad, " %*s |", subscriptionStateWidth, "Subscription");
    // wprintw(pad, " %*s |", fifoWidth, "FiFo"); // uncomment once this information is needed
    wprintw(pad, " %*s\n", scopeWidth, "Propagation");

    wprintw(pad, " %*s |", serviceWidth, "");
    wprintw(pad, " %*s |", instanceWidth, "");
    wprintw(pad, " %*s |", eventWidth, "");
    wprintw(pad, " %*s |", runtimeNameWidth, "");
    wprintw(pad, " %*s |", nodeNameWidth, "");
    wprintw(pad, " %*s |", subscriptionStateWidth, "State");
    // wprintw(pad, " %*s |", fifoWidth, "size / capacity"); // uncomment once this information is needed
    wprintw(pad, " %*s\n", scopeWidth, "scope");

    wprintw(pad, "---------------------------------------------------------------------------------------------------");
    wprintw(pad, "---------------------------------------------------\n");

    auto subscriptionStateToString = [](iox::SubscribeState subState) -> std::string {
        switch (subState)
        {
        case iox::SubscribeState::NOT_SUBSCRIBED:
            return "NOT_SUBSCRIBED";
        case iox::SubscribeState::SUBSCRIBE_REQUESTED:
            return "SUB_REQUEST";
        case iox::SubscribeState::SUBSCRIBED:
            return "SUBSCRIBED";
        case iox::SubscribeState::UNSUBSCRIBE_REQUESTED:
            return "UNSUB_REQUEST";
        case iox::SubscribeState::WAIT_FOR_OFFER:
            return "WAIT_FOR_OFFER";
        default:
            return "UNKNOWN";
        }
    };

    for (auto& subscriber : subscriberPortData)
    {
        currentLine = 0;
        do
        {
            needsLineBreak = false;
            wprintw(pad, " %s |", printEntry(serviceWidth, subscriber.portData->m_caproServiceID).c_str());
            wprintw(pad, " %s |", printEntry(instanceWidth, subscriber.portData->m_caproInstanceID).c_str());
            wprintw(pad, " %s |", printEntry(eventWidth, subscriber.portData->m_caproEventMethodID).c_str());
            wprintw(pad, " %s |", printEntry(runtimeNameWidth, subscriber.portData->m_name).c_str());
            wprintw(pad, " %s |", printEntry(nodeNameWidth, subscriber.portData->m_node).c_str());
            wprintw(pad,
                    " %s |",
                    printEntry(subscriptionStateWidth,
                               subscriptionStateToString(subscriber.subscriberPortChangingData->subscriptionState))
                        .c_str());
            // uncomment once this information is needed
            // if (currentLine == 0)
            //{
            // std::string fifoSize{"n/a"};     // std::to_string(subscriber.subscriberPortChangingData->fifoSize))
            // std::string fifoCapacity{"n/a"}; // std::to_string(subscriber.subscriberPortChangingData->fifoCapacity))
            // wprintw(pad,
            //" %s / %s |",
            // printEntry(((fifoWidth / 2) - 1), fifoSize).c_str(),
            // printEntry(((fifoWidth / 2) - 1), fifoCapacity).c_str());
            //}
            // else
            //{
            // wprintw(pad, " %*s |", fifoWidth, "");
            //}
            wprintw(pad,
                    " %s\n",
                    printEntry(scopeWidth,
                               std::string(capro::ScopeTypeString[static_cast<std::underlying_type<capro::Scope>::type>(
                                   subscriber.subscriberPortChangingData->propagationScope)]))
                        .c_str());

            currentLine++;
        } while (needsLineBreak);

        wprintw(pad, " %*s |", serviceWidth, "");
        wprintw(pad, " %*s |", instanceWidth, "");
        wprintw(pad, " %*s |", eventWidth, "");
        wprintw(pad, " %*s |", runtimeNameWidth, "");
        wprintw(pad, " %*s |", nodeNameWidth, "");
        wprintw(pad, " %*s |", subscriptionStateWidth, "");
        // wprintw(pad, " %*s |", fifoWidth, ""); // uncomment once this information is needed
        wprintw(pad, " %*s", scopeWidth, "");
        wprintw(pad, "\n");
    }
}

template <typename Subscriber>
bool IntrospectionApp::waitForSubscription(Subscriber& port)
{
    uint32_t numberOfLoopsTillTimeout{100};
    bool subscribed{false};
    while ((subscribed = (port.getSubscriptionState() == iox::SubscribeState::SUBSCRIBED)),
           !subscribed && numberOfLoopsTillTimeout > 0)
    {
        numberOfLoopsTillTimeout--;
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_INTERVAL.toMilliseconds()));
    }

    return subscribed;
}

std::vector<ComposedPublisherPortData>
IntrospectionApp::composePublisherPortData(const PortIntrospectionFieldTopic* portData,
                                           const PortThroughputIntrospectionFieldTopic* throughputData)
{
    std::vector<ComposedPublisherPortData> publisherPortData;
    publisherPortData.reserve(portData->m_publisherList.size());

    const PortThroughputData dummyThroughputData;

    auto& m_publisherList = portData->m_publisherList;
    auto& m_throughputList = throughputData->m_throughputList;
    const bool fastLookup = (m_publisherList.size() == m_throughputList.size());
    for (uint64_t i = 0u; i < m_publisherList.size(); ++i)
    {
        bool found = (fastLookup && m_publisherList[i].m_publisherPortID == m_throughputList[i].m_publisherPortID);
        if (found)
        {
            publisherPortData.push_back({m_publisherList[i], m_throughputList[i]});
            continue;
        }
        else
        {
            for (const auto& throughput : m_throughputList)
            {
                if (m_publisherList[i].m_publisherPortID == throughput.m_publisherPortID)
                {
                    publisherPortData.push_back({m_publisherList[i], throughput});
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                publisherPortData.push_back({m_publisherList[i], dummyThroughputData});
            }
        }
    }

    auto publisherSortCriterion = [](const ComposedPublisherPortData& publisher1,
                                     const ComposedPublisherPortData& publisher2) {
        std::string name1(publisher1.portData->m_name.c_str());
        std::string name2(publisher2.portData->m_name.c_str());
        return name1.compare(name2) < 0;
    };
    std::sort(publisherPortData.begin(), publisherPortData.end(), publisherSortCriterion);

    return publisherPortData;
}

std::vector<ComposedSubscriberPortData> IntrospectionApp::composeSubscriberPortData(
    const PortIntrospectionFieldTopic* portData,
    const SubscriberPortChangingIntrospectionFieldTopic* subscriberPortChangingData)
{
    std::vector<ComposedSubscriberPortData> subscriberPortData;
    subscriberPortData.reserve(portData->m_subscriberList.size());

    uint32_t i = 0U;
    if (portData->m_subscriberList.size() == subscriberPortChangingData->subscriberPortChangingDataList.size())
    { // should be the same, else it will be soon
        for (const auto& port : portData->m_subscriberList)
        {
            subscriberPortData.push_back({port, subscriberPortChangingData->subscriberPortChangingDataList[i++]});
        }
    }

    auto subscriberSortCriterion = [](const ComposedSubscriberPortData& subscriber1,
                                      const ComposedSubscriberPortData& subscriber2) {
        std::string name1(subscriber1.portData->m_name.c_str());
        std::string name2(subscriber2.portData->m_name.c_str());
        return name1.compare(name2) < 0;
    };
    std::sort(subscriberPortData.begin(), subscriberPortData.end(), subscriberSortCriterion);

    return subscriberPortData;
}

void IntrospectionApp::runIntrospection(const iox::units::Duration updatePeriod,
                                        const IntrospectionSelection introspectionSelection)
{
    iox::runtime::PoshRuntime::initRuntime(iox::roudi::INTROSPECTION_APP_NAME);

    using namespace iox::roudi;

    initTerminal();
    prettyPrint("### Iceoryx Introspection Client ###\n\n", PrettyOptions::title);

    popo::SubscriberOptions subscriberOptions;
    subscriberOptions.queueCapacity = 1U;
    subscriberOptions.historyRequest = 1U;

    // mempool
    iox::popo::Subscriber<MemPoolIntrospectionInfoContainer> memPoolSubscriber(IntrospectionMempoolService,
                                                                               subscriberOptions);
    if (introspectionSelection.mempool == true)
    {
        memPoolSubscriber.subscribe();

        if (waitForSubscription(memPoolSubscriber) == false)
        {
            prettyPrint("Timeout while waiting for subscription for mempool introspection data!\n",
                        PrettyOptions::error);
        }
    }

    // process
    iox::popo::Subscriber<ProcessIntrospectionFieldTopic> processSubscriber(IntrospectionProcessService,
                                                                            subscriberOptions);
    if (introspectionSelection.process == true)
    {
        processSubscriber.subscribe();

        if (waitForSubscription(processSubscriber) == false)
        {
            prettyPrint("Timeout while waiting for subscription for process introspection data!\n",
                        PrettyOptions::error);
        }
    }

    // port
    iox::popo::Subscriber<PortIntrospectionFieldTopic> portSubscriber(IntrospectionPortService, subscriberOptions);
    iox::popo::Subscriber<PortThroughputIntrospectionFieldTopic> portThroughputSubscriber(
        IntrospectionPortThroughputService, subscriberOptions);
    iox::popo::Subscriber<SubscriberPortChangingIntrospectionFieldTopic> subscriberPortChangingDataSubscriber(
        IntrospectionSubscriberPortChangingDataService, subscriberOptions);

    if (introspectionSelection.port == true)
    {
        portSubscriber.subscribe();
        portThroughputSubscriber.subscribe();
        subscriberPortChangingDataSubscriber.subscribe();

        if (waitForSubscription(portSubscriber) == false)
        {
            prettyPrint("Timeout while waiting for subscription for port introspection data!\n", PrettyOptions::error);
        }
        if (waitForSubscription(portThroughputSubscriber) == false)
        {
            prettyPrint("Timeout while waiting for subscription for port throughput introspection data!\n",
                        PrettyOptions::error);
        }
        if (waitForSubscription(subscriberPortChangingDataSubscriber) == false)
        {
            prettyPrint("Timeout while waiting for Subscription for Subscriber Port Introspection Changing Data!\n",
                        PrettyOptions::error);
        }
    }

    // Refresh once in case of timeout messages
    refreshTerminal();

    cxx::optional<popo::Sample<const MemPoolIntrospectionInfoContainer>> memPoolSample;
    cxx::optional<popo::Sample<const ProcessIntrospectionFieldTopic>> processSample;
    cxx::optional<popo::Sample<const PortIntrospectionFieldTopic>> portSample;
    cxx::optional<popo::Sample<const PortThroughputIntrospectionFieldTopic>> portThroughputSample;
    cxx::optional<popo::Sample<const SubscriberPortChangingIntrospectionFieldTopic>> subscriberPortChangingDataSamples;

    while (true)
    {
        // get and print time
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        char timeBuf[128];
        strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %X", std::localtime(&in_time_t));
        prettyPrint("### Iceoryx Introspection Client ### ", PrettyOptions::title);
        prettyPrint(timeBuf, PrettyOptions::bold);
        prettyPrint("\n\n", PrettyOptions::bold);

        // print mempool information
        if (introspectionSelection.mempool == true)
        {
            prettyPrint("### MemPool Status ###\n\n", PrettyOptions::highlight);

            memPoolSubscriber.take().and_then([&](auto& sample) { memPoolSample = sample; });

            if (memPoolSample)
            {
                for (const auto& i : *(memPoolSample.value().get()))
                {
                    printMemPoolInfo(i);
                }
            }
            else
            {
                prettyPrint("Waiting for mempool introspection data ...\n");
            }
        }

        // print process information
        if (introspectionSelection.process == true)
        {
            prettyPrint("### Processes ###\n\n", PrettyOptions::highlight);
            processSubscriber.take().and_then([&](auto& sample) { processSample = sample; });

            if (processSample)
            {
                printProcessIntrospectionData(processSample.value().get());
            }
            else
            {
                prettyPrint("Waiting for process introspection data ...\n");
            }
        }

        // print port information
        if (introspectionSelection.port == true)
        {
            portSubscriber.take().and_then([&](auto& sample) { portSample = sample; });

            portThroughputSubscriber.take().and_then([&](auto& sample) { portThroughputSample = sample; });

            subscriberPortChangingDataSubscriber.take().and_then(
                [&](auto& sample) { subscriberPortChangingDataSamples = sample; });

            if (portSample && portThroughputSample && subscriberPortChangingDataSamples)
            {
                prettyPrint("### Connections ###\n\n", PrettyOptions::highlight);
                auto composedPublisherPortData =
                    composePublisherPortData(portSample.value().get(), portThroughputSample.value().get());
                auto composedSubscriberPortData = composeSubscriberPortData(
                    portSample.value().get(), subscriberPortChangingDataSamples.value().get());

                printPortIntrospectionData(composedPublisherPortData, composedSubscriberPortData);
            }
            else
            {
                prettyPrint("Waiting for port introspection data ...\n");
            }
        }

        prettyPrint("\n");
        clearToBottom();
        refreshTerminal();

        // Watch user input for updatePeriodMs
        auto tWaitRemaining = std::chrono::milliseconds(updatePeriod.toMilliseconds());
        auto tWaitBegin = std::chrono::system_clock::now();
        while (tWaitRemaining.count() >= 0)
        {
            waitForUserInput(static_cast<int32_t>(tWaitRemaining.count()));
            auto tWaitElapsed = std::chrono::system_clock::now() - tWaitBegin;
            tWaitRemaining = std::chrono::milliseconds(updatePeriod.toMilliseconds())
                             - std::chrono::duration_cast<std::chrono::milliseconds>(tWaitElapsed);
        }
    }

    getchar();
    closeTerminal();
}

} // namespace introspection
} // namespace client
} // namespace iox
