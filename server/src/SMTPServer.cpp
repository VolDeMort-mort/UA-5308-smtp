#include "SMTPServer.h"
#include <iostream>

using boost::asio::ip::tcp;

SMTPServer::SMTPServer(boost::asio::io_context& ioc, unsigned short port, 
	const std::string& cert_file, const std::string& key_file)
	: m_ioc(ioc),
	m_acceptor(ioc, tcp::endpoint(tcp::v4(), port)),
	m_work_guard(boost::asio::make_work_guard(m_ioc))
{
	if (!cert_file.empty() && !key_file.empty()){
		try{
			m_ssl_ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tls_server);

			m_ssl_ctx->set_options(boost::asio::ssl::context::default_workarounds |
								   boost::asio::ssl::context::no_sslv2 | 
								   boost::asio::ssl::context::no_sslv3);

			m_ssl_ctx->use_private_key_file(key_file, boost::asio::ssl::context::pem);
			m_ssl_ctx->use_certificate_chain_file(cert_file);

			m_has_tls = true;
		}
		catch (const std::exception& ex)
		{
			// log
			m_ssl_ctx.reset();
			m_has_tls = false;
		}
	}
}

SMTPServer::~SMTPServer() {
	if (!m_running) return;
	m_running = false;

	boost::system::error_code ec;
	m_acceptor.cancel(ec);
	m_acceptor.close(ec);

	m_work_guard.reset();
	m_ioc.stop();

	for (auto& t : m_threads) {
		if (t.joinable()) t.join();
	}
	m_threads.clear();
}

void SMTPServer::run(std::size_t threads) {
	if (m_running) return;
	m_running = true;

	do_accept();

	std::size_t nt = std::max<std::size_t>(1, threads);
	m_threads.reserve(nt);
	for (std::size_t i = 0; i < nt; ++i) {
		m_threads.emplace_back([this]() {
			try {
				m_ioc.run();
			}
			catch (std::exception& ex) {
				//log
			}
			});
	}
}

void SMTPServer::soft_stop() {
	if (!m_running) return;
	m_running = false;

	boost::system::error_code ec;
	m_acceptor.cancel(ec);
	m_acceptor.close(ec);

	m_work_guard.reset();

	for (auto& t : m_threads) {
		if (t.joinable()) t.join();
	}
	m_threads.clear();
}

void SMTPServer::do_accept() {
	m_acceptor.async_accept([this](const boost::system::error_code& ec, tcp::socket socket) mutable {
		if (!ec) {
			try {
				auto serverSocket = std::make_shared<SMTPServerSocket>(std::move(socket));
				auto session = std::make_shared<SMTPServerSession>(serverSocket, m_has_tls ? m_ssl_ctx : nullptr);

				session->run();
			}
			catch (std::exception& ex) {
				//log
			}
		}
		else {
			//log
		}

		if (m_running) {
			do_accept();
		}
		});
}
