#pragma once
#include <string>
#include <vector>

struct PurchasedTicket {
    int         posterIdx;
    std::string hall;
    std::string row;
    std::string seat;
    std::string fmt;
    std::string time;
    std::string dayName;
    int         dayNum;
};

// Filled by movieDetail when Buy is clicked; consumed by purchasePage on confirm
struct SeatRef { int row; int col; };
inline std::vector<SeatRef>        g_pendingSeats;

inline std::vector<PurchasedTicket> g_purchasedTickets;
