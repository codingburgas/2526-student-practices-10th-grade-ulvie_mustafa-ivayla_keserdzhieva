#pragma once
#include "../../cinema_data/include/ITicketRepository.h"
#include <string>
#include <memory>

// Abstract ID generator interface so business logic remains pure
class IIdGenerator {
public:
    virtual ~IIdGenerator() = default;
    virtual std::string GenerateId() = 0;
};

class BookingService {
private:
    std::shared_ptr<ITicketRepository> m_repository;
    std::shared_ptr<IIdGenerator> m_idGenerator;

public:
    BookingService(std::shared_ptr<ITicketRepository> repo, std::shared_ptr<IIdGenerator> idGen)
        : m_repository(repo), m_idGenerator(idGen) {
    }

    std::string BookMovie(const std::string& movieName) {
        // Core Business Rule: Verify movie is valid, then create booking ID
        if (movieName.empty()) return "";

        std::string ticketId = m_idGenerator->GenerateId();

        if (m_repository->SaveTicket(ticketId, movieName)) {
            return ticketId;
        }
        return "";
    }
};