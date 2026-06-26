USE CinemaDB;
GO

-- Add Movies and Shows tables only (safe to run without touching Users or Tickets)
IF OBJECT_ID('Shows',  'U') IS NOT NULL DROP TABLE Shows;
IF OBJECT_ID('Movies', 'U') IS NOT NULL DROP TABLE Movies;
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
