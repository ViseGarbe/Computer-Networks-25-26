-- =========================
-- DROP TABLES
-- =========================
DROP TABLE IF EXISTS order_item_ingredients CASCADE;
DROP TABLE IF EXISTS product_ingredients CASCADE;
DROP TABLE IF EXISTS ingredients CASCADE;
DROP TABLE IF EXISTS order_items CASCADE;
DROP TABLE IF EXISTS payments CASCADE;
DROP TABLE IF EXISTS orders CASCADE;
DROP TABLE IF EXISTS products CASCADE;
DROP TABLE IF EXISTS categories CASCADE;
DROP TABLE IF EXISTS users CASCADE;

-- =========================
-- USERS
-- =========================
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    first_name VARCHAR(100) NOT NULL,
    last_name VARCHAR(100) NOT NULL,
    email VARCHAR(150) UNIQUE NOT NULL,
    password VARCHAR(255) NOT NULL,
    role VARCHAR(20) NOT NULL DEFAULT 'CLIENT'
        CHECK (role IN ('CLIENT', 'ADMIN')),
    phone VARCHAR(20),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Only one admin with fixed email
ALTER TABLE users
ADD CONSTRAINT only_one_admin CHECK (
    (role = 'ADMIN' AND email = 'admin@bar.com') OR role = 'CLIENT'
);

-- =========================
-- CATEGORIES
-- =========================
CREATE TABLE categories (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL UNIQUE
);

-- =========================
-- PRODUCTS
-- =========================
CREATE TABLE products (
    id SERIAL PRIMARY KEY,
    name VARCHAR(150) NOT NULL,
    description TEXT,
    price DECIMAL(6,2) NOT NULL CHECK (price > 0),
    category_id INT,
    image_url TEXT,

    CONSTRAINT fk_category
        FOREIGN KEY (category_id)
        REFERENCES categories(id)
        ON DELETE SET NULL
);

-- =========================
-- INGREDIENTS
-- =========================
CREATE TABLE ingredients (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL UNIQUE,
    is_removable BOOLEAN DEFAULT TRUE,
    extra_price DECIMAL(5,2) DEFAULT 0 CHECK (extra_price >= 0)
);

-- =========================
-- PRODUCT_INGREDIENTS (M:N)
-- =========================
CREATE TABLE product_ingredients (
    product_id INT NOT NULL,
    ingredient_id INT NOT NULL,
    quantity NUMERIC DEFAULT 1,

    PRIMARY KEY (product_id, ingredient_id),

    CONSTRAINT fk_pi_product
        FOREIGN KEY (product_id)
        REFERENCES products(id)
        ON DELETE CASCADE,

    CONSTRAINT fk_pi_ingredient
        FOREIGN KEY (ingredient_id)
        REFERENCES ingredients(id)
        ON DELETE CASCADE
);

-- =========================
-- ORDERS
-- =========================
CREATE TABLE orders (
    id SERIAL PRIMARY KEY,
    user_id INT NOT NULL,
    total DECIMAL(6,2) NOT NULL CHECK (total >= 0),
    type VARCHAR(20) NOT NULL
        CHECK (type IN ('TAKEAWAY', 'TABLE')),

    CONSTRAINT fk_user
        FOREIGN KEY (user_id)
        REFERENCES users(id)
        ON DELETE CASCADE
);

-- =========================
-- ORDER ITEMS
-- =========================
CREATE TABLE order_items (
    id SERIAL PRIMARY KEY,
    order_id INT NOT NULL,
    product_id INT NOT NULL,
    quantity INT NOT NULL CHECK (quantity > 0), -- Ex. 2 coffees
    unit_price DECIMAL(5,2) NOT NULL CHECK (unit_price > 0),

    CONSTRAINT fk_order
        FOREIGN KEY (order_id)
        REFERENCES orders(id)
        ON DELETE CASCADE,

    CONSTRAINT fk_product
        FOREIGN KEY (product_id)
        REFERENCES products(id)
        ON DELETE CASCADE
);

-- =========================
-- ORDER ITEM INGREDIENTS (customization)
-- =========================
CREATE TABLE order_item_ingredients (
    order_item_id INT NOT NULL,
    ingredient_id INT NOT NULL,
    action VARCHAR(10) NOT NULL CHECK (action IN ('ADD', 'REMOVE')),

    PRIMARY KEY (order_item_id, ingredient_id),

    FOREIGN KEY (order_item_id)
        REFERENCES order_items(id)
        ON DELETE CASCADE,

    FOREIGN KEY (ingredient_id)
        REFERENCES ingredients(id)
        ON DELETE CASCADE
);

-- =========================
-- PAYMENTS
-- =========================
CREATE TABLE payments (
    id SERIAL PRIMARY KEY,
    order_id INT NOT NULL,
    method VARCHAR(20) NOT NULL
        CHECK (method IN ('CASH', 'CARD', 'ONLINE')),
    status VARCHAR(20) NOT NULL DEFAULT 'PENDING'
        CHECK (status IN ('PAID', 'PENDING')),
    paid_at TIMESTAMP,

    CONSTRAINT fk_payment_order
        FOREIGN KEY (order_id)
        REFERENCES orders(id)
        ON DELETE CASCADE
);

-- =========================
-- INDEXES
-- =========================
CREATE INDEX idx_users_email ON users(email);
CREATE INDEX idx_orders_user ON orders(user_id);
CREATE INDEX idx_order_items_order ON order_items(order_id);
CREATE INDEX idx_order_items_product ON order_items(product_id);

-- =========================
-- DATA INSERTION
-- =========================

-- Categories
INSERT INTO categories (name) VALUES
('Drinks'),
('Sweets'),
('Sandwiches'),
('Cold Beverages'),
('Specials');

-- Products (NO quantity anymore)
INSERT INTO products (name, description, price, category_id) VALUES
-- Drinks
('Espresso', 'Strong Italian coffee', 1.20, 1),
('Cappuccino', 'Coffee with steamed milk', 1.50, 1),
('Latte Macchiato', 'Layered coffee with milk', 1.80, 1),

-- Sweets
('Croissant', 'Buttery morning pastry', 1.20, 2),
('Chocolate Muffin', 'Rich chocolate muffin', 1.50, 2),
('Cheesecake', 'Classic cream cheesecake', 2.50, 2),

-- Sandwiches
('Ham Sandwich', 'Fresh ham and cheese', 3.50, 3),
('Veggie Sandwich', 'Grilled vegetables and cheese', 3.00, 3),

-- Cold Beverages
('Orange Juice', 'Freshly squeezed', 2.00, 4),
('Iced Tea', 'Lemon flavored', 1.80, 4),

-- Specials
('Pumpkin Latte', 'Seasonal pumpkin spice latte', 2.50, 5);

-- Ingredients
INSERT INTO ingredients (name, is_removable, extra_price) VALUES
('Bread', FALSE, 0),
('Ham', TRUE, 0),
('Cheese', TRUE, 0.50),
('Tomato', TRUE, 0),
('Lettuce', TRUE, 0),
('Mushrooms', TRUE, 0.40);

-- Product ↔ Ingredients
INSERT INTO product_ingredients (product_id, ingredient_id) VALUES
(7, 1), -- Bread
(7, 2), -- Ham
(7, 3), -- Cheese
(7, 4), -- Tomato

(8, 1), -- Bread
(8, 3), -- Cheese
(8, 4), -- Tomato
(8, 5); -- Lettuce

-- Users
INSERT INTO users (first_name, last_name, email, password, role) VALUES 
('Admin', 'Bar', 'admin@bar.com', 'HASHED_PASSWORD', 'ADMIN'),
('Client1', 'Bar', 'client1@bar.com', 'HASHED_PASSWORD', 'CLIENT'),
('Client2', 'Bar', 'client2@bar.com', 'HASHED_PASSWORD', 'CLIENT'),
('Mario', 'Rossi', 'mario@email.com', 'HASHED_PASSWORD', 'CLIENT'),
('Luisa', 'Bianchi', 'luisa@email.com', 'HASHED_PASSWORD', 'CLIENT');

-- Orders
INSERT INTO orders (user_id, total, type) VALUES
(2, 4.20, 'TAKEAWAY'),
(3, 6.50, 'TABLE');

-- Order Items
INSERT INTO order_items (order_id, product_id, quantity, unit_price) VALUES
(1, 1, 2, 1.20),
(1, 4, 1, 1.20),
(2, 7, 1, 3.50);

-- Customization example
INSERT INTO order_item_ingredients (order_item_id, ingredient_id, action) VALUES
(3, 4, 'REMOVE'), -- remove tomato
(3, 3, 'ADD');    -- extra cheese

-- Payments
INSERT INTO payments (order_id, method, status, paid_at) VALUES
(1, 'CASH', 'PAID', CURRENT_TIMESTAMP),
(2, 'CARD', 'PENDING', NULL);