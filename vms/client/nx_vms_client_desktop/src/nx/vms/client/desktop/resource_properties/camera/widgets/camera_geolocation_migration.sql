-- Database migration for camera geolocation fields
-- Adds latitude, longitude, altitude, and geolocation_address fields to camera table

-- For MySQL
ALTER TABLE camera ADD COLUMN latitude REAL DEFAULT NULL;
ALTER TABLE camera ADD COLUMN longitude REAL DEFAULT NULL;
ALTER TABLE camera ADD COLUMN altitude REAL DEFAULT NULL;
ALTER TABLE camera ADD COLUMN geolocation_address TEXT DEFAULT NULL;

-- For SQLite (alternative syntax)
-- ALTER TABLE camera ADD COLUMN latitude REAL;
-- ALTER TABLE camera ADD COLUMN longitude REAL;
-- ALTER TABLE camera ADD COLUMN altitude REAL;
-- ALTER TABLE camera ADD COLUMN geolocation_address TEXT;
