#pragma once
#include "../../cinema_data/include/ITicketRepository.h"
#include <string>
#include <memory>

class IIdGenerator {
public:
    virtual ~IIdGenerator() = default;
    virtual std::string GenerateId() = 0;
};

class BookingService {
    std::shared_ptr<ITicketRepository> m_repository;
    std::shared_ptr<IIdGenerator>      m_idGenerator;
public:
    BookingService(std::shared_ptr<ITicketRepository> repo,
                   std::shared_ptr<IIdGenerator>      idGen)
        : m_repository(repo), m_idGenerator(idGen) {}

    // Fills ticketId, persists to DB, returns the generated ID (empty on failure).
    std::string BookTicket(TicketRecord record) {
        if (record.title.empty()) return "";
        record.ticketId = m_idGenerator->GenerateId();
        return m_repository->SaveTicket(record) ? record.ticketId : "";
    }
};
