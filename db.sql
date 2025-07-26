-- Entities table
CREATE TABLE entities (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    address TEXT,
    logo BYTEA,
    creation_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Users table
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    entity_id INTEGER REFERENCES entities(id) ON DELETE CASCADE,
    first_name VARCHAR(50) NOT NULL,
    last_name VARCHAR(100) NOT NULL,
    username VARCHAR(30) UNIQUE NOT NULL,
    password VARCHAR(255) NOT NULL,  -- Will be stored encrypted
    is_admin BOOLEAN DEFAULT FALSE,
    creation_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT chk_username CHECK (username ~ '^[a-zA-Z0-9_]+$')
);

-- Audit logs table
CREATE TABLE audit_logs (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id) ON DELETE SET NULL,
    action VARCHAR(20) NOT NULL,  -- CREATE, UPDATE, DELETE, LOGIN, etc.
    affected_table VARCHAR(30),
    record_id INTEGER,
    previous_data JSONB,
    new_data JSONB,
    event_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Original risk system tables
CREATE TABLE assets (
    id SERIAL PRIMARY KEY,
    entity_id INTEGER REFERENCES entities(id) ON DELETE CASCADE,
    description VARCHAR(255) NOT NULL,
    type VARCHAR(2) NOT NULL CHECK (type IN ('HW', 'SW', 'DT', 'PR', 'DO')),
    location VARCHAR(255),
    functionality INTEGER CHECK (functionality BETWEEN 0 AND 10),
    cost INTEGER CHECK (cost BETWEEN 0 AND 10),
    image INTEGER CHECK (image BETWEEN 0 AND 10),
    confidentiality INTEGER CHECK (confidentiality BETWEEN 0 AND 10),
    integrity INTEGER CHECK (integrity BETWEEN 0 AND 10),
    availability INTEGER CHECK (availability BETWEEN 0 AND 10),
    value NUMERIC(5,2) GENERATED ALWAYS AS (
        (COALESCE(functionality, 0) +
        COALESCE(cost, 0) +
        COALESCE(image, 0) +
        COALESCE(confidentiality, 0) +
        COALESCE(integrity, 0) +
        COALESCE(availability, 0)
    )::NUMERIC / 6) STORED,
    created_by INTEGER REFERENCES users(id) ON DELETE SET NULL,
    creation_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE threats (
    id SERIAL PRIMARY KEY,
    description VARCHAR(255) NOT NULL,
    created_by INTEGER REFERENCES users(id) ON DELETE SET NULL,
    creation_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE assets_threats (
    asset_id INTEGER REFERENCES assets(id) ON DELETE CASCADE,
    threat_id INTEGER REFERENCES threats(id) ON DELETE CASCADE,
    probability NUMERIC(2,1) CHECK (probability BETWEEN 0 AND 1),
    PRIMARY KEY (asset_id, threat_id),
    modified_by INTEGER REFERENCES users(id) ON DELETE SET NULL,
    modification_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Risk Views
CREATE OR REPLACE VIEW risks AS
SELECT
    a.id AS asset_id,
    a.description AS asset,
    AVG(at.probability) AS risk,
    a.value AS importance,
    AVG(at.probability) * a.value AS risk_weight,
    a.entity_id
FROM
    assets a
JOIN
    assets_threats at ON a.id = at.asset_id
GROUP BY
    a.id, a.description, a.value, a.entity_id;

CREATE OR REPLACE VIEW total_risk AS
SELECT
    entity_id,
    SUM(risk_weight) / NULLIF(SUM(importance), 0) AS system_total_risk
FROM
    risks
GROUP BY
    entity_id;

-- Functions for user management and auditing
CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE OR REPLACE FUNCTION register_action(
    p_user_id INTEGER,
    p_action VARCHAR,
    p_table VARCHAR,
    p_record_id INTEGER,
    p_previous_data JSONB,
    p_new_data JSONB
) RETURNS VOID AS $$
BEGIN
    INSERT INTO audit_logs(
        user_id, action, affected_table, record_id, previous_data, new_data
    ) VALUES (
        p_user_id, p_action, p_table, p_record_id, p_previous_data, p_new_data
    );
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION encrypt_password(p_password VARCHAR)
RETURNS VARCHAR AS $$
BEGIN
    RETURN crypt(p_password, gen_salt('bf', 12));
END;
$$ LANGUAGE plpgsql STRICT;

-- Triggers for automatic auditing
CREATE OR REPLACE FUNCTION audit_asset_changes()
RETURNS TRIGGER AS $$
BEGIN
    IF (TG_OP = 'DELETE') THEN
        PERFORM register_action(
            OLD.created_by, 'DELETE', 'assets',
            OLD.id, to_jsonb(OLD), NULL
        );
    ELSIF (TG_OP = 'UPDATE') THEN
        PERFORM register_action(
            NEW.created_by, 'UPDATE', 'assets',
            NEW.id, to_jsonb(OLD), to_jsonb(NEW)
        );
    ELSIF (TG_OP = 'INSERT') THEN
        PERFORM register_action(
            NEW.created_by, 'CREATE', 'assets',
            NEW.id, NULL, to_jsonb(NEW)
        );
    END IF;
    RETURN NULL;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trigger_asset_audit
AFTER INSERT OR UPDATE OR DELETE ON assets
FOR EACH ROW EXECUTE FUNCTION audit_asset_changes();

CREATE OR REPLACE FUNCTION verify_password(
    p_username VARCHAR,
    p_entered_password VARCHAR
) RETURNS BOOLEAN AS $$
DECLARE
    v_stored_password VARCHAR;
BEGIN
    SELECT password INTO v_stored_password
    FROM users
    WHERE username = p_username;

    RETURN v_stored_password = crypt(p_entered_password, v_stored_password);
END;
$$ LANGUAGE plpgsql STRICT;
