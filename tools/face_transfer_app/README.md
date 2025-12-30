# Face Transfer App

This is a host-side application to detect faces and transfer them to the S300 development board via UART.

## Prerequisites

- Python 3.8+
- [uv](https://github.com/astral-sh/uv) (Python package manager)
- A webcam
- S300 Development Board connected via UART (default COM7)

## Setup

1.  **Install uv** (if not already installed):
    ```powershell
    # Windows (PowerShell)
    powershell -c "irm https://astral.sh/uv/install.ps1 | iex"
    ```

2.  **Install dependencies**:
    Navigate to this directory in PowerShell:
    ```powershell
    cd tools/face_transfer_app
    uv sync
    ```

## Usage

1.  **Connect S300 Board**:
    Ensure the S300 board is running the `s300_uart_file_transfer` firmware and connected to the PC.
    Check the COM port number in Device Manager. If it's not `COM7`, edit `app.py` to change `SERIAL_PORT`.

2.  **Run the App**:
    ```powershell
    uv run python app.py
    ```

3.  **Controls**:
    - **'t'**: Capture current face as **Target Face** and send to S300.
    - **'c'**: Capture current face as **Compare Face** and send to S300.
    - **'q'**: Quit the application.

## Troubleshooting

- **Serial Port Error**: Ensure the COM port is correct and not used by another application (like a serial terminal).
- **No Face Detected**: Ensure lighting is good and you are facing the camera.
