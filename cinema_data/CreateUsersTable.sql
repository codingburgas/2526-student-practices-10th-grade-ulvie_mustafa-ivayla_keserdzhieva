USE CinemaDB;
GO

-- Drop old tables if they exist (re-run safe)
IF OBJECT_ID('PendingAdminVerifications', 'U') IS NOT NULL DROP TABLE PendingAdminVerifications;
IF OBJECT_ID('Users', 'U')                      IS NOT NULL DROP TABLE Users;
GO

CREATE TABLE Users (
    Id        INT           IDENTITY(1,1) PRIMARY KEY,
    Name      NVARCHAR(256) NOT NULL,
    Surname   NVARCHAR(256) NOT NULL,
    Username  NVARCHAR(256) NOT NULL UNIQUE,
    Password  NVARCHAR(256) NOT NULL,
    Email     NVARCHAR(256) NULL,
    Role      NVARCHAR(50)  NOT NULL DEFAULT 'customer',   -- 'customer' | 'admin'
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
