package com.uuidgenerator.lib.dualbuffer;

import com.uuidgenerator.lib.IdGenerator;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.locks.ReentrantLock;

public class DualBufferGenerator implements IdGenerator {
    private Connection connection;
    private final IdBuffer[] buffers = new IdBuffer[2];
    private final AtomicInteger activeIdx = new AtomicInteger(0);
    private final ReentrantLock mutex = new ReentrantLock();
    private final AtomicBoolean fetchSignal = new AtomicBoolean(false);

    private static class IdBuffer {
        long currentId;
        long maxId;
        volatile boolean ready = false;
    }

    public DualBufferGenerator() {
        buffers[0] = new IdBuffer();
        buffers[1] = new IdBuffer();

        try {
            Class.forName("com.mysql.cj.jdbc.Driver");
            String url = "jdbc:mysql://mysql-dual-buffer:3306/uuid_db?useSSL=false&allowPublicKeyRetrieval=true";
            String user = "root";
            String password = "rootpassword";

            for (int i = 0; i < 30; i++) {
                try {
                    connection = DriverManager.getConnection(url, user, password);
                    connection.setAutoCommit(false);
                    System.out.println("Successfully connected to MySQL.");
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

        fetchNextBlock(0);
        if (!buffers[0].ready) {
            System.err.println("Failed to fetch initial ID segment from database");
            System.exit(1);
        }

        new Thread(this::backgroundFetcher).start();
    }

    private void fetchNextBlock(int bufferIdx) {
        try {
            long maxId = 0;
            long step = 0;

            String selectQuery = "SELECT max_id, step FROM id_generator WHERE biz_tag = 'user' FOR UPDATE";
            try (PreparedStatement selectStmt = connection.prepareStatement(selectQuery);
                 ResultSet rs = selectStmt.executeQuery()) {
                if (rs.next()) {
                    maxId = rs.getLong("max_id");
                    step = rs.getLong("step");
                } else {
                    System.err.println("Error: No row found for biz_tag = 'user'");
                    connection.rollback();
                    return;
                }
            }

            long newMaxId = maxId + step;

            String updateQuery = "UPDATE id_generator SET max_id = ? WHERE biz_tag = 'user'";
            try (PreparedStatement updateStmt = connection.prepareStatement(updateQuery)) {
                updateStmt.setLong(1, newMaxId);
                updateStmt.executeUpdate();
            }

            connection.commit();

            mutex.lock();
            try {
                buffers[bufferIdx].currentId = maxId;
                buffers[bufferIdx].maxId = newMaxId;
                buffers[bufferIdx].ready = true;
            } finally {
                mutex.unlock();
            }

            System.out.println("Fetched new block for buffer " + bufferIdx + ": [" + maxId + ", " + newMaxId + ")");
        } catch (Exception e) {
            System.err.println("Error fetching next block: " + e.getMessage());
            try {
                connection.rollback();
            } catch (Exception rollbackEx) {
                System.err.println("Error rolling back transaction: " + rollbackEx.getMessage());
            }
        }
    }

    private void backgroundFetcher() {
        while (true) {
            if (fetchSignal.compareAndSet(true, false)) {
                int standbyIdx = 1 - activeIdx.get();
                fetchNextBlock(standbyIdx);
            } else {
                try {
                    Thread.sleep(10);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    break;
                }
            }
        }
    }

    @Override
    public String nextIdString() {
        mutex.lock();
        try {
            int currentActiveIdx = activeIdx.get();
            IdBuffer activeBuffer = buffers[currentActiveIdx];

            if (activeBuffer.currentId >= activeBuffer.maxId) {
                int standbyIdx = 1 - currentActiveIdx;
                IdBuffer standbyBuffer = buffers[standbyIdx];

                while (!standbyBuffer.ready) {
                    mutex.unlock();
                    try {
                        Thread.sleep(10);
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                        return "";
                    }
                    mutex.lock();
                }

                activeIdx.set(standbyIdx);
                activeBuffer.ready = false;
                activeBuffer = standbyBuffer;
            }

            long id = activeBuffer.currentId++;

            long threshold = activeBuffer.maxId - (long) ((activeBuffer.maxId - id) * 0.9);
            if (activeBuffer.currentId == threshold) {
                fetchSignal.set(true);
            }

            return String.valueOf(id);
        } finally {
            mutex.unlock();
        }
    }
}
