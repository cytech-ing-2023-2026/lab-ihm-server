CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    name VARCHAR(100) NOT NULL,
    password VARCHAR(97) NOT NULL,
    admin BOOLEAN NOT NULL
);

CREATE TABLE IF NOT EXISTS messages (
    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    sender INTEGER NOT NULL,
    receiver INTEGER,
    content VARCHAR(2000) NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (sender) REFERENCES users(id),
    FOREIGN KEY (receiver) REFERENCES users(id)
);

-- 
-- Argon2id
-- User: admin
-- Password: admin
INSERT OR IGNORE INTO users (id, name, password, admin) VALUES (
  0,
  'admin',
  '$argon2id$v=19$m=65536,t=2,p=1$Ib6cOs8SiSrqrh1yT73PoQ$SKtzHnh16339ZdooY2VB/BqihcxH9D2CNgHIJaYO2ew',
  1
);

--
-- Default user
-- User: jordan
-- Password: admin
INSERT OR IGNORE INTO users (id, name, password, admin) VALUES (
  1,
  'jordan',
  '$argon2id$v=19$m=65536,t=2,p=1$Ib6cOs8SiSrqrh1yT73PoQ$SKtzHnh16339ZdooY2VB/BqihcxH9D2CNgHIJaYO2ew',
  0
);

--
-- Default user
-- User: francois
-- Password: admin
INSERT OR IGNORE INTO users (id, name, password, admin) VALUES (
  2,
  'francois',
  '$argon2id$v=19$m=65536,t=2,p=1$Ib6cOs8SiSrqrh1yT73PoQ$SKtzHnh16339ZdooY2VB/BqihcxH9D2CNgHIJaYO2ew',
  0
);