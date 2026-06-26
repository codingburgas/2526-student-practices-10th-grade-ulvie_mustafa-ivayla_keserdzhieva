USE CinemaDB;
GO

-- Drop old tables if they exist (re-run safe)
IF OBJECT_ID('Tickets',                   'U') IS NOT NULL DROP TABLE Tickets;
IF OBJECT_ID('Shows',                     'U') IS NOT NULL DROP TABLE Shows;
IF OBJECT_ID('Movies',                    'U') IS NOT NULL DROP TABLE Movies;
IF OBJECT_ID('PendingAdminVerifications', 'U') IS NOT NULL DROP TABLE PendingAdminVerifications;
IF OBJECT_ID('Users',                     'U') IS NOT NULL DROP TABLE Users;
GO

CREATE TABLE Users (
    Id        INT           IDENTITY(1,1) PRIMARY KEY,
    Name      NVARCHAR(256) NOT NULL,
    Surname   NVARCHAR(256) NOT NULL,
    Username  NVARCHAR(256) NOT NULL UNIQUE,
    Password  NVARCHAR(256) NOT NULL,
    Email     NVARCHAR(256) NULL,
    Role      NVARCHAR(50)  NOT NULL DEFAULT 'customer',
    Company   NVARCHAR(256) NULL,
    City      NVARCHAR(256) NULL,
    CreatedAt DATETIME      NOT NULL DEFAULT GETDATE()
);
GO

CREATE TABLE PendingAdminVerifications (
    Id        INT           IDENTITY(1,1) PRIMARY KEY,
    Email     NVARCHAR(256) NOT NULL,
    Code      NVARCHAR(10)  NOT NULL,
    ExpiresAt DATETIME      NOT NULL,
    Used      BIT           NOT NULL DEFAULT 0
);
GO

CREATE TABLE Movies (
    Id          INT            IDENTITY(1,1) PRIMARY KEY,
    Title       NVARCHAR(256)  NOT NULL,
    Genre       NVARCHAR(100)  NOT NULL DEFAULT '',
    Year        INT            NOT NULL DEFAULT 2024,
    Description NVARCHAR(1000) NOT NULL DEFAULT '',
    PosterFile  NVARCHAR(256)  NOT NULL DEFAULT '',
    CreatedAt   DATETIME       NOT NULL DEFAULT GETDATE()
);
GO

CREATE TABLE Shows (
    Id        INT           IDENTITY(1,1) PRIMARY KEY,
    MovieId   INT           NOT NULL REFERENCES Movies(Id) ON DELETE CASCADE,
    Hall      NVARCHAR(10)  NOT NULL,
    Format    NVARCHAR(20)  NOT NULL DEFAULT '2D',
    Time1     NVARCHAR(20)  NOT NULL DEFAULT '',
    Time2     NVARCHAR(20)  NOT NULL DEFAULT '',
    Time3     NVARCHAR(20)  NOT NULL DEFAULT '',
    Time4     NVARCHAR(20)  NOT NULL DEFAULT '',
    CreatedAt DATETIME      NOT NULL DEFAULT GETDATE()
);
GO

CREATE TABLE Tickets (
    Id          INT           IDENTITY(1,1) PRIMARY KEY,
    TicketId    NVARCHAR(64)  NOT NULL UNIQUE,
    Title       NVARCHAR(256) NOT NULL,
    Hall        NVARCHAR(10)  NOT NULL,
    Seat        NVARCHAR(10)  NOT NULL,
    Format      NVARCHAR(20)  NOT NULL,
    ShowTime    NVARCHAR(20)  NOT NULL,
    ShowDay     INT           NOT NULL,
    ShowMonth   INT           NOT NULL,
    ShowYear    INT           NOT NULL,
    TicketType  NVARCHAR(50)  NOT NULL,
    City        NVARCHAR(256) NOT NULL DEFAULT 'Burgas',
    Location    NVARCHAR(256) NOT NULL DEFAULT 'Galleria Burgas',
    PurchasedAt DATETIME      NOT NULL DEFAULT GETDATE()
);
GO
