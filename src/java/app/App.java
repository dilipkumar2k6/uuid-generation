import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.Socket;

public class App {
    public static void main(String[] args) {
        System.out.println("App container starting with 5 concurrent threads...");

        int numThreads = 5;
        for (int i = 1; i <= numThreads; i++) {
            final int threadId = i;
            new Thread(() -> requestUUID(threadId)).start();
        }
    }

    private static void requestUUID(int threadId) {
        while (true) {
            try (Socket socket = new Socket("127.0.0.1", 8080);
                 BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()))) {
                
                String uuid = in.readLine();
                if (uuid != null) {
                    synchronized (System.out) {
                        System.out.println("[Thread " + threadId + "] Received UUID: " + uuid);
                    }
                } else {
                    synchronized (System.err) {
                        System.err.println("[Thread " + threadId + "] Failed to read UUID");
                    }
                }
            } catch (Exception e) {
                synchronized (System.err) {
                    System.err.println("[Thread " + threadId + "] Connection Failed. Retrying...");
                }
            }

            try {
                Thread.sleep(500);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                break;
            }
        }
    }
}
