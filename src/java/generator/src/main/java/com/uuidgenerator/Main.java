package com.uuidgenerator;

import com.uuidgenerator.lib.IdGenerator;
import com.uuidgenerator.lib.dbautoinc.DbAutoIncGenerator;
import com.uuidgenerator.lib.dualbuffer.DualBufferGenerator;
import com.uuidgenerator.lib.etcdsnowflake.EtcdSnowflake;
import com.uuidgenerator.lib.hlcsnowflake.HlcSnowflake;
import com.uuidgenerator.lib.instasnowflake.InstaSnowflake;
import com.uuidgenerator.lib.snowflake.Snowflake;
import com.uuidgenerator.lib.sonyflake.Sonyflake;
import com.uuidgenerator.lib.spanner.SpannerGenerator;
import com.uuidgenerator.lib.spannertruetime.SpannerTrueTimeGenerator;
import com.uuidgenerator.lib.uuidv4.UuidV4Generator;
import com.uuidgenerator.lib.uuidv7.UuidV7Generator;

import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.charset.StandardCharsets;

public class Main {
    public static void main(String[] args) {
        // ---------------------------------------------------------
        // 1. Determine Generator Type
        // ---------------------------------------------------------
        String genType = System.getenv("GENERATOR_TYPE");
        if (genType == null || genType.isEmpty()) {
            genType = "SNOWFLAKE";
        }

        IdGenerator generator;

        switch (genType) {
            case "HLC_SNOWFLAKE":
                System.out.println("Initializing HLC Snowflake generator...");
                generator = new HlcSnowflake();
                break;
            case "INSTA_SNOWFLAKE":
                System.out.println("Initializing Instagram Snowflake generator...");
                generator = new InstaSnowflake();
                break;
            case "SONYFLAKE":
                System.out.println("Initializing Sonyflake generator...");
                generator = new Sonyflake();
                break;
            case "UUIDV4":
                System.out.println("Initializing UUID Version 4 generator...");
                generator = new UuidV4Generator();
                break;
            case "UUIDV7":
                System.out.println("Initializing UUID Version 7 generator...");
                generator = new UuidV7Generator();
                break;
            case "DB_AUTO_INC":
                System.out.println("Initializing Database Auto-Increment generator...");
                generator = new DbAutoIncGenerator();
                break;
            case "DUAL_BUFFER":
                System.out.println("Initializing Dual Buffer generator...");
                generator = new DualBufferGenerator();
                break;
            case "ETCD_SNOWFLAKE":
                System.out.println("Initializing Etcd-Coordinated Snowflake generator...");
                generator = new EtcdSnowflake();
                break;
            case "SPANNER":
                System.out.println("Initializing Spanner Sequence generator...");
                generator = new SpannerGenerator();
                break;
            case "SPANNER_TRUETIME":
                System.out.println("Initializing Spanner TrueTime generator...");
                generator = new SpannerTrueTimeGenerator();
                break;
            default:
                System.out.println("Initializing Standard Snowflake generator...");
                generator = new Snowflake();
                break;
        }

        // ---------------------------------------------------------
        // 2. Setup TCP Server Socket
        // ---------------------------------------------------------
        try (ServerSocket serverSocket = new ServerSocket(8080)) {
            System.out.println("Sidecar listening on port 8080...");

            // ---------------------------------------------------------
            // 3. Main Server Loop
            // ---------------------------------------------------------
            while (true) {
                try {
                    // Accept an incoming connection
                    Socket clientSocket = serverSocket.accept();

                    // Generate a new UUID string and send it to the connected client
                    String uuidStr = generator.nextIdString();
                    OutputStream out = clientSocket.getOutputStream();
                    out.write(uuidStr.getBytes(StandardCharsets.UTF_8));
                    out.flush();

                    // Close the connection immediately after sending (stateless IPC)
                    clientSocket.close();
                } catch (Exception e) {
                    System.err.println("Accept failed: " + e.getMessage());
                }
            }
        } catch (Exception e) {
            System.err.println("Listen failed: " + e.getMessage());
            System.exit(1);
        }
    }
}
