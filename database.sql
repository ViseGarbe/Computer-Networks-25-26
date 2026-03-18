-- Drop all child tables first
DROP TABLE IF EXISTS ingredients CASCADE;
DROP TABLE IF EXISTS order_items CASCADE;
DROP TABLE IF EXISTS payments CASCADE;

-- Then parent tables
DROP TABLE IF EXISTS orders CASCADE;
DROP TABLE IF EXISTS products CASCADE;
DROP TABLE IF EXISTS categories CASCADE;
DROP TABLE IF EXISTS users CASCADE;

-- USERS
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

-- CATEGORIES (used to organize products into logical groups ex: Drinks	Sweets Sandwiches)
CREATE TABLE categories (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL UNIQUE
);

-- PRODUCTS
CREATE TABLE products (
    id SERIAL PRIMARY KEY,
    name VARCHAR(150) NOT NULL,
    description TEXT,
    price DECIMAL(6,2) NOT NULL CHECK (price > 0),
    category_id INT,
    -- quantity INT NOT NULL CHECK (quantity > 0), -- Since it's an ordering system, we don't need to track inventory 
    image_url TEXT,

    
    CONSTRAINT fk_category
        FOREIGN KEY (category_id)
        REFERENCES categories(id)
        ON DELETE SET NULL
);


-- ORDERS
CREATE TABLE orders (
    id SERIAL PRIMARY KEY,
    user_id INT NOT NULL,
    total DECIMAL(6,2) NOT NULL,
    type VARCHAR(20) NOT NULL
        CHECK (type IN ('TAKEAWAY', 'TABLE')),

    CONSTRAINT fk_user
        FOREIGN KEY (user_id)
        REFERENCES users(id)
        ON DELETE CASCADE
);


-- ORDER ITEMS
CREATE TABLE order_items (
    id SERIAL PRIMARY KEY,
    order_id INT NOT NULL,
    product_id INT NOT NULL,
    quantity INT NOT NULL CHECK (quantity > 0),
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


-- PAYMENTS
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

-- INDEXES
CREATE INDEX idx_users_email ON users(email);
CREATE INDEX idx_orders_user ON orders(user_id);
CREATE INDEX idx_order_items_order ON order_items(order_id);
CREATE INDEX idx_order_items_product ON order_items(product_id);

ALTER TABLE users
ADD CONSTRAINT only_one_admin CHECK (
    (role = 'ADMIN' AND email = 'admin@bar.com') OR role = 'CLIENT'
);

INSERT INTO categories (name) VALUES
('Drinks'),
('Sweets'),
('Sandwiches'),
('Cold Beverages'),
('Specials');

INSERT INTO products (name, description, price, category_id, quantity) VALUES
-- Drinks
('Espresso', 'Strong Italian coffee', 1.20, 1, 10),
('Cappuccino', 'Coffee with steamed milk', 1.50, 1, 10),
('Latte Macchiato', 'Layered coffee with milk', 1.80, 1, 10),

-- Sweets
('Croissant', 'Buttery morning pastry', 1.20, 2, 10),
('Chocolate Muffin', 'Rich chocolate muffin', 1.50, 2, 10),
('Cheesecake', 'Classic cream cheesecake', 2.50, 2, 10),

-- Sandwiches
('Ham Sandwich', 'Fresh ham and cheese', 3.50, 3, 10),
('Veggie Panini', 'Grilled vegetables and cheese', 3.00, 3, 10),

-- Cold Beverages
('Orange Juice', 'Freshly squeezed', 2.00, 4, 10),
('Iced Tea', 'Lemon flavored', 1.80, 4, 10),

-- Specials
('Pumpkin Latte', 'Seasonal pumpkin spice latte', 2.50, 5, 10);

-- Admin
INSERT INTO users (first_name, last_name, email, password, role)
VALUES 
('Admin', 'Bar', 'admin@bar.com', 'HASHED_PASSWORD', 'ADMIN'),
('Client1', 'Bar', 'client1@bar.com', 'HASHED_PASSWORD', 'CLIENT'),
('Client2', 'Bar', 'client2@bar.com', 'HASHED_PASSWORD', 'CLIENT'),
('Mario', 'Rossi', 'mario@email.com', 'HASHED_PASSWORD', 'CLIENT'),
('Luisa', 'Bianchi', 'luisa@email.com', 'HASHED_PASSWORD', 'CLIENT');

INSERT INTO orders (user_id, total, type) VALUES
(2, 4.20, 'TAKEAWAY'),
(3, 6.50, 'TABLE');

INSERT INTO payments (order_id, method, status, paid_at) VALUES
(1, 'CASH', 'PAID', CURRENT_TIMESTAMP),
(2, 'CARD', 'PENDING', NULL);