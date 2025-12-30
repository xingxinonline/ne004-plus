import cv2
import serial
import struct
import time
import mediapipe as mp
import numpy as np
import sys
import threading
import queue

import re

# Configuration
SERIAL_PORT = 'COM7'  # Default, can be changed
BAUD_RATE = 921600
FACE_SIZE = (112, 112)
CONSOLE_WIDTH = 400  # Width of the debug console panel


class FaceTransferApp:
    def __init__(self):
        self.cap = cv2.VideoCapture(0)
        if not self.cap.isOpened():
            print("Error: Could not open camera.")
            sys.exit(1)

        # MediaPipe Face Detection
        self.mp_face_detection = mp.solutions.face_detection
        self.face_detection = self.mp_face_detection.FaceDetection(
            model_selection=0, min_detection_confidence=0.5)

        self.serial_port = None
        self.similarity_score = None  # Store similarity score
        self.best_match_id = None     # Store best match target ID
        self.target_count = 0         # Number of registered targets

        try:
            self.serial_port = serial.Serial(
                SERIAL_PORT, BAUD_RATE, timeout=0.1)  # Short timeout for non-blocking read loop
            print(f"Opened serial port {SERIAL_PORT}")
        except Exception as e:
            print(f"Error opening serial port {SERIAL_PORT}: {e}")
            print("Running in offline mode (no transfer).")

        self.running = True
        self.status_message = "Press 't' to save Target, 'c' to save Compare, 'q' to quit."

        # Console buffer
        self.console_lines = []
        self.current_line = ""
        self.console_lock = threading.Lock()

        # Serial handling
        self.serial_queue = queue.Queue()
        self.is_transferring = False
        self.reader_thread = threading.Thread(
            target=self.read_serial_loop, daemon=True)
        self.reader_thread.start()

    def log_to_console(self, text):
        with self.console_lock:
            for char in text:
                if char == '\n':
                    self.console_lines.append(self.current_line)
                    self.current_line = ""
                elif char == '\r':
                    pass
                else:
                    self.current_line += char

            # Keep buffer size reasonable
            if len(self.console_lines) > 100:
                self.console_lines = self.console_lines[-100:]

    def read_serial_loop(self):
        while self.running:
            if self.serial_port and self.serial_port.is_open:
                try:
                    if self.serial_port.in_waiting:
                        data = self.serial_port.read(
                            self.serial_port.in_waiting)
                        if self.is_transferring:
                            for b in data:
                                self.serial_queue.put(bytes([b]))
                        else:
                            # Print debug info directly
                            text = data.decode(errors='ignore')
                            print(text, end='', flush=True)
                            self.log_to_console(text)

                            # Parse similarity score (new format: [SIM] Best Match: ID=X, Score=Y (of Z targets))
                            match = re.search(
                                r'\[SIM\] Best Match: ID=(\d+), Score=(\d+) \(of (\d+) targets\)', text)
                            if match:
                                self.best_match_id = int(match.group(1))
                                self.similarity_score = int(match.group(2))
                                self.target_count = int(match.group(3))
                                print(
                                    f"\n[PC] Best Match: ID={self.best_match_id}, Score={self.similarity_score}% (of {self.target_count} targets)\n")
                    else:
                        time.sleep(0.01)
                except Exception as e:
                    # print(f"Serial read error: {e}")
                    time.sleep(0.1)
            else:
                time.sleep(0.1)

    def wait_for_ack(self, timeout=5):
        start_time = time.time()
        while time.time() - start_time < timeout:
            try:
                byte = self.serial_queue.get(timeout=0.1)
                if byte == b'K':
                    return True
                elif byte == b'E':
                    return False  # NAK - error from MCU
                else:
                    # Debug info from MCU
                    text = byte.decode(errors='ignore')
                    print(text, end='', flush=True)
                    self.log_to_console(text)
            except queue.Empty:
                continue
        return False

    def send_face_data(self, face_img, command):
        if not self.serial_port or not self.serial_port.is_open:
            self.status_message = "Serial port not open!"
            return

        self.is_transferring = True
        # Clear queue
        while not self.serial_queue.empty():
            self.serial_queue.get()

        try:
            # Convert to RGB
            face_rgb = cv2.cvtColor(face_img, cv2.COLOR_BGR2RGB)
            data = face_rgb.tobytes()
            size = len(data)
            chunk_size = 4096
            total_chunks = (size + chunk_size - 1) // chunk_size

            face_name = "Target" if command == 'target' else "Compare"
            addr = "DSP SRAM0 @ 0x44000000"

            msg = f"\n[PC] ========== Starting Transfer ==========\n"
            print(msg, end='')
            self.log_to_console(msg)

            msg = f"[PC] Face: {face_name} | Size: {size} bytes | Chunks: {total_chunks}\n"
            print(msg, end='')
            self.log_to_console(msg)

            msg = f"[PC] Destination: Flash @ {addr}\n"
            print(msg, end='')
            self.log_to_console(msg)
            self.status_message = f"Sending {face_name}..."

            # 0. Enter Transfer Mode
            msg = f"[PC] Step 0: Sending 'load' command...\n"
            print(msg, end='')
            self.log_to_console(msg)

            self.serial_port.write(b'load\r')
            time.sleep(0.5)  # Wait for MCU to switch modes
            # Clear queue again to remove "Starting..." messages
            while not self.serial_queue.empty():
                self.serial_queue.get()
            # 1. Send Command
            cmd_byte = b'T' if command == 'target' else b'C'
            msg = f"[PC] Step 1: Sending command '{cmd_byte.decode()}' (0x{cmd_byte[0]:02X})...\n"
            print(msg, end='')
            self.log_to_console(msg)

            self.serial_port.write(cmd_byte)

            msg = f"[PC]   -> Waiting for MCU ACK...\n"
            print(msg, end='')
            self.log_to_console(msg)

            if not self.wait_for_ack(timeout=3):
                raise Exception("No ACK for command")

            msg = f"[PC]   <- Received ACK 'K'\n"
            print(msg, end='')
            self.log_to_console(msg)

            # 2. Send Size
            msg = f"[PC] Step 2: Sending size {size} bytes (0x{size:08X})...\n"
            print(msg, end='')
            self.log_to_console(msg)

            self.serial_port.write(struct.pack('<I', size))

            msg = f"[PC]   -> Waiting for MCU to erase flash...\n"
            print(msg, end='')
            self.log_to_console(msg)

            # Wait for erase ACK (can take time)
            start_erase = time.time()
            if not self.wait_for_ack(timeout=10):
                raise Exception("No ACK after erase")
            erase_time = time.time() - start_erase

            msg = f"[PC]   <- Flash erased (took {erase_time:.2f}s)\n"
            print(msg, end='')
            self.log_to_console(msg)

            # 3. Send Data Chunks
            msg = f"[PC] Step 3: Transferring {total_chunks} chunks...\n"
            print(msg, end='')
            self.log_to_console(msg)

            start_transfer = time.time()

            for i in range(0, size, chunk_size):
                chunk_num = i // chunk_size
                chunk = data[i:i+chunk_size]
                chunk_len = len(chunk)

                msg = f"[PC]   Chunk {chunk_num}/{total_chunks}: {chunk_len} bytes @ offset 0x{i:05X}... "
                print(msg, end='', flush=True)
                self.log_to_console(msg)

                self.serial_port.write(chunk)

                # Wait for chunk ACK
                if not self.wait_for_ack(timeout=3):
                    raise Exception(f"No ACK for chunk {chunk_num}")

                msg = f"ACK\n"
                print(msg, end='')
                self.log_to_console(msg)

                # Progress
                progress = (chunk_num + 1) * 100 // total_chunks
                self.status_message = f"Sending {face_name}... {progress}%"

            transfer_time = time.time() - start_transfer
            speed = size / transfer_time / 1024  # KB/s

            msg = f"[PC] ========== Transfer Complete ==========\n"
            print(msg, end='')
            self.log_to_console(msg)

            msg = f"[PC] Time: {transfer_time:.2f}s | Speed: {speed:.1f} KB/s\n"
            print(msg, end='')
            self.log_to_console(msg)

            msg = f"[PC] {face_name} face saved to Flash @ {addr}\n\n"
            print(msg, end='')
            self.log_to_console(msg)

            self.status_message = f"{face_name} face saved!"

        except Exception as e:
            msg = f"\n[PC] ========== Transfer Failed ==========\n"
            print(msg, end='')
            self.log_to_console(msg)

            msg = f"[PC] Error: {e}\n\n"
            print(msg, end='')
            self.log_to_console(msg)

            self.status_message = f"Failed: {e}"
        finally:
            self.is_transferring = False

    def run(self):
        while self.running:
            ret, frame = self.cap.read()
            if not ret:
                break

            # Flip for mirror effect
            frame = cv2.flip(frame, 1)
            h, w, _ = frame.shape

            # Detect faces
            frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            results = self.face_detection.process(frame_rgb)

            face_img_to_send = None

            if results.detections:
                for detection in results.detections:
                    bboxC = detection.location_data.relative_bounding_box
                    ih, iw, _ = frame.shape
                    x, y, w_box, h_box = int(bboxC.xmin * iw), int(bboxC.ymin * ih), \
                        int(bboxC.width * iw), int(bboxC.height * ih)

                    # Draw bounding box
                    cv2.rectangle(frame, (x, y), (x + w_box,
                                  y + h_box), (0, 255, 0), 2)

                    # Prepare crop if needed
                    # Ensure coordinates are within bounds
                    x = max(0, x)
                    y = max(0, y)
                    w_box = min(w_box, iw - x)
                    h_box = min(h_box, ih - y)

                    if w_box > 0 and h_box > 0:
                        face_crop = frame[y:y+h_box, x:x+w_box]
                        try:
                            face_img_to_send = cv2.resize(face_crop, FACE_SIZE)
                        except:
                            pass

            # Create combined canvas
            canvas = np.zeros((h, w + CONSOLE_WIDTH, 3), dtype=np.uint8)
            canvas[:h, :w, :] = frame

            # Draw status on video
            cv2.putText(canvas, self.status_message, (10, 30),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)

            # Draw Similarity Score if available
            if self.similarity_score is not None:
                # Line 1: Best Match ID and Score
                score_text = f"Best Match: ID={self.best_match_id}, Score={self.similarity_score}%"
                # Line 2: Target count
                count_text = f"Targets: {self.target_count}"

                # Calculate text sizes
                (text_w1, text_h1), _ = cv2.getTextSize(
                    score_text, cv2.FONT_HERSHEY_SIMPLEX, 0.8, 2)
                (text_w2, text_h2), _ = cv2.getTextSize(
                    count_text, cv2.FONT_HERSHEY_SIMPLEX, 0.6, 1)

                box_w = max(text_w1, text_w2) + 20
                box_h = text_h1 + text_h2 + 25

                # Draw background box
                cv2.rectangle(canvas, (10, 50),
                              (10 + box_w, 50 + box_h), (0, 0, 0), -1)

                # Draw score text
                color = (0, 255, 0) if self.similarity_score > 70 else (
                    0, 0, 255)
                cv2.putText(canvas, score_text, (15, 50 + text_h1 + 5),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.8, color, 2)
                # Draw target count
                cv2.putText(canvas, count_text, (15, 50 + text_h1 + text_h2 + 15),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.6, (200, 200, 200), 1)

            # Draw console
            # Draw separator line
            cv2.line(canvas, (w, 0), (w, h), (100, 100, 100), 1)

            # Draw console text
            with self.console_lock:
                lines_to_draw = self.console_lines + [self.current_line]

            font_scale = 0.4
            font_thickness = 1
            line_height = 15
            max_lines = (h - 20) // line_height

            start_line = max(0, len(lines_to_draw) - max_lines)

            y_pos = 20
            for i in range(start_line, len(lines_to_draw)):
                line = lines_to_draw[i]
                # Simple clipping if too long
                if len(line) > 50:
                    line = line[:47] + "..."
                cv2.putText(canvas, line, (w + 10, y_pos),
                            cv2.FONT_HERSHEY_SIMPLEX, font_scale, (0, 255, 0), font_thickness)
                y_pos += line_height

            cv2.imshow('Face Transfer App', canvas)

            key = cv2.waitKey(1) & 0xFF
            if key == ord('q'):
                self.running = False
            elif key == ord('t'):
                if face_img_to_send is not None:
                    if not self.is_transferring:
                        threading.Thread(target=self.send_face_data, args=(
                            face_img_to_send, 'target')).start()
                    else:
                        self.status_message = "Transfer in progress..."
            elif key == ord('c'):
                if face_img_to_send is not None:
                    if not self.is_transferring:
                        threading.Thread(target=self.send_face_data, args=(
                            face_img_to_send, 'compare')).start()
                    else:
                        self.status_message = "Transfer in progress..."

        self.cap.release()
        if self.serial_port:
            self.serial_port.close()
        cv2.destroyAllWindows()


if __name__ == "__main__":
    app = FaceTransferApp()
    app.run()
