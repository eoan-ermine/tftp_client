#include <boost/asio.hpp>

#include <tftp_common/packets.hpp>
#include <tftp_common/serialization.hpp>
#include <tftp_common/parsers.hpp>

#include <string>
#include <fstream>
#include <iostream>

using boost::asio::ip::udp;

namespace tftp_client {

template <typename Iterator, typename Packet, typename ParseFunction>
bool try_parse(Iterator begin, Iterator end, Packet& packet, ParseFunction parseFunction) {
	bool result = parseFunction(begin, end, packet);
	if (result) return true;
	else {
		tftp_common::packets::error err_packet;
		bool result = tftp_common::parsers::parse_error_packet(begin, end, err_packet);
		if (result) throw err_packet;
	}
	return false;
}

class TFTPClient {
	udp::resolver& resolver;
	udp::endpoint &receiver_endpoint, sender_endpoint;
	udp::socket& socket;
	std::vector<unsigned char> send_buffer, recv_buffer;
public:
	TFTPClient(udp::resolver& resolver, udp::endpoint& receiver_endpoint, udp::socket& socket)
		: resolver(resolver), receiver_endpoint(receiver_endpoint), socket(socket), send_buffer(1024), recv_buffer(1024) { }

	void send(std::string fromPath, std::string toPath, std::string transferMode) {
		std::ifstream file{fromPath};
		if (!file && file.eof()) return;

		std::size_t packet_size = tftp_common::serialize(tftp_common::packets::wrq {
			.filename = toPath,
			.mode = transferMode
		}, send_buffer.begin());
		socket.send_to(boost::asio::buffer(send_buffer, packet_size), receiver_endpoint);
		std::size_t bytesRead = socket.receive_from(boost::asio::buffer(recv_buffer), sender_endpoint);

		std::string new_port = std::to_string(sender_endpoint.port());
		receiver_endpoint = *resolver.resolve(udp::v4(), receiver_endpoint.address().to_string(), new_port).begin();

		tftp_common::packets::ack acknowledgment_packet;
		tftp_common::packets::data current_packet;
		uint16_t current_block = 0;

		while (file && !file.eof()) {
			try_parse(recv_buffer.begin(), recv_buffer.begin() + bytesRead, acknowledgment_packet, [](auto begin, auto end, auto& packet) {
				return tftp_common::parsers::parse_ack_packet(begin, end, packet);
			});

			std::vector<unsigned char> data_buffer(512);
			std::size_t packet_size;

			/*
				Новый пакет данных следует формировать лишь в том случае, если
				предыдущий был отправлен успешно, иначе следует отправлять прошлый
				пакет данных
			*/
			if (acknowledgment_packet.block == current_block) {
				current_block += 1;

				file.read(reinterpret_cast<char*>(data_buffer.data()), 512);
				data_buffer.resize(file.gcount());
				packet_size = tftp_common::serialize({
					.block = current_block, .data = std::move(data_buffer)
				}, send_buffer.begin());
			}

			socket.send_to(boost::asio::buffer(send_buffer, packet_size), receiver_endpoint);
			bytesRead = socket.receive_from(boost::asio::buffer(recv_buffer), sender_endpoint);
		}
	}

	void read(std::string fromPath, std::string toPath, std::string transferMode) {
		std::ofstream file{toPath};
		if (!file) return;

		std::size_t packet_size = tftp_common::serialize(tftp_common::packets::rrq {
			.filename = fromPath,
			.mode = transferMode
		}, send_buffer.begin());
		socket.send_to(boost::asio::buffer(send_buffer, packet_size), receiver_endpoint);
		std::size_t bytesRead = socket.receive_from(boost::asio::buffer(recv_buffer), sender_endpoint);

		std::string new_port = std::to_string(sender_endpoint.port());
		receiver_endpoint = *resolver.resolve(udp::v4(), receiver_endpoint.address().to_string(), new_port).begin();

		tftp_common::packets::data data_packet;
		uint16_t current_block = 1;

		while (true) {
			try_parse(recv_buffer.begin(), recv_buffer.begin() + bytesRead, data_packet, [](auto begin, auto end, auto& packet) {
				return tftp_common::parsers::parse_data_packet(begin, end, packet);
			});

			std::size_t packet_size;

			/*
				Новый пакет данных следует формировать лишь в том случае, если
				предыдущий был отправлен успешно, иначе следует отправлять прошлый
				пакет данных
			*/
			if (data_packet.block == current_block) {
				current_block += 1;

				file.write(reinterpret_cast<char*>(data_packet.data.data()), data_packet.data.size());
				packet_size = tftp_common::serialize(tftp_common::packets::ack {
					.block = data_packet.block
				}, send_buffer.begin());
			}

			socket.send_to(boost::asio::buffer(send_buffer, packet_size), receiver_endpoint);
			if (data_packet.data.size() != 512) break;
			bytesRead = socket.receive_from(boost::asio::buffer(recv_buffer), sender_endpoint);
		}
	}
};

}