package com.uuidgenerator.lib.dbautoinc;

import com.uuidgenerator.lib.IdGenerator;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.Statement;

public class DbAutoIncGenerator implements IdGenerator {
    private Connection connection;

    public DbAutoIncGenerator() {
        try {
            Class.forName("com.mysql.cj.jdbc.Driver");
            String url = "jdbc:mysql://proxysql:6033/uuid_db?useSSL=false&allowPublicKeyRetrieval=true";
            String user = "root";
            String password = "rootpassword";

            for (int i = 0; i < 30; i++) {
                try {
                    connection = DriverManager.getConnection(url, user, password);
                    System.out.println("Successfully connected to ProxySQL.");
                    break;
                } catch (Exception e) {
                    System.out.println("Waiting for database connection...");
                    Thread.sleep(2000);
                }
            }

            if (connection == null) {
                System.err.println("Database connection failed after retries.");
                System.exit(1);
            }
        } catch (Exception e) {
            System.err.println("Failed to initialize database connection: " + e.getMessage());
            System.exit(1);
        }
    }

    @Override
    public String nextIdString() {
        String query = "REPLACE INTO tickets64 (stub) VALUES ('a')";
        try (PreparedStatement pstmt = connection.prepareStatement(query, Statement.RETURN_GENERATED_KEYS)) {
            pstmt.executeUpdate();
            try (ResultSet rs = pstmt.getGeneratedKeys()) {
                if (rs.next()) {
                    long id = rs.getLong(1);
                    return String.valueOf(id);
                }
            }
        } catch (Exception e) {
            System.err.println("Error executing query: " + e.getMessage());
        }
        return "";
    }
}
