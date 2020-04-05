#include "gtest/gtest.h"
#include <marlin/simulator/core/Simulator.hpp>
#include <marlin/simulator/transport/SimulatedTransportFactory.hpp>
#include <marlin/simulator/network/Network.hpp>

#include <marlin/beacon/DiscoveryClient.hpp>

#include "spdlog/fmt/bin_to_hex.h"

#include <cstring>

using namespace marlin::core;
using namespace marlin::asyncio;
using namespace marlin::simulator;
using namespace marlin::beacon;


struct Delegate {
	std::function<
		std::vector<std::tuple<uint32_t, uint16_t, uint16_t>>()
	> get_protocols = []() -> std::vector<std::tuple<uint32_t, uint16_t, uint16_t>> {
		return {};
	};

	std::function<void(SocketAddress const&, uint8_t const*, uint32_t, uint16_t)> new_peer = [](
		SocketAddress const& addr,
		uint8_t const* static_pk,
		uint32_t protocol,
		uint16_t version
	) {
		SPDLOG_INFO(
			"New peer: {}, {:spn}, {}, {}",
			addr.to_string(),
			spdlog::to_hex(static_pk, static_pk+32),
			protocol,
			version
		);
	};
};

TEST(DiscoveryClientTest, Constructible) {
	uint8_t static_sk[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk[crypto_box_PUBLICKEYBYTES];
	crypto_box_keypair(static_pk, static_sk);

	DiscoveryClient<Delegate> client(SocketAddress::loopback_ipv4(8000), static_sk);
}


struct DiscoveryClientProtocolTest : public ::testing::Test {
	template<typename Delegate>
	using TransportType = SimulatedTransport<
		Simulator,
		NetworkInterface<Network<NetworkConditioner>>,
		Delegate
	>;
	template<typename ListenDelegate, typename TransportDelegate>
	using TransportFactoryType = SimulatedTransportFactory<
		Simulator,
		NetworkInterface<Network<NetworkConditioner>>,
		ListenDelegate,
		TransportDelegate
	>;

	Simulator simulator;
	NetworkConditioner nc;
	Network<NetworkConditioner> network;

	DiscoveryClientProtocolTest() : network(nc) {}
};

TEST_F(DiscoveryClientProtocolTest, SendsHeartbeatWhenDiscoverable) {
	auto& i1 = network.get_or_create_interface(SocketAddress::from_string("192.168.0.1:0"));
	// auto& i2 = network.get_or_create_interface(SocketAddress::from_string("192.168.0.2:0"));

	uint8_t static_sk[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk[crypto_box_PUBLICKEYBYTES];
	crypto_box_keypair(static_pk, static_sk);

	DiscoveryClient<Delegate, TransportFactoryType, TransportType> client(
		SocketAddress::from_string("192.168.0.1:8002"),
		static_sk,
		i1,
		simulator
	);
	client.is_discoverable = true;
	client.start_discovery(SocketAddress::from_string("192.168.0.2:8002"));

	simulator.run();
}
