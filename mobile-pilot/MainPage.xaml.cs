using System.Net.Sockets;

namespace Xmas_Led_Ctrl
{
    public partial class MainPage : ContentPage
    {
        private readonly TcpClientService _tcpClient;
        // Define byte streams for each button (these can be customized later)
        private readonly Dictionary<int, byte[]> _buttonCommands = new()
        {
            { 1, new byte[] { 0x01, 0x03, 0x00, 0x05, 0x00, 0x90, 0x01, 0x14, 0x00, 0xdc } },  // rand-slow
            { 2, new byte[] { 0x01, 0x03, 0x00, 0x05, 0x00, 0x28, 0x00, 0x14, 0x00, 0xc8 } },  // rand-fast
            { 3, new byte[] { 0x01, 0x0c, 0x00, 0x0b, 0x00, 0x19, 0x00, 0x96, 0x00, 0x00, 0x00, 0x68, 0x01, 0x01, 0x00, 0xe6 } },  // rwalk-fast
            { 4, new byte[] { 0x01, 0x0c, 0x00, 0x0b, 0x00, 0x19, 0x00, 0x64, 0x00, 0x00, 0x00, 0x68, 0x01, 0x01, 0x00, 0xc8 } },  // rwalk-slow
            { 5, new byte[] { 0x01, 0x0c, 0x00, 0x0b, 0x00, 0x32, 0x00, 0x96, 0x00, 0xf0, 0x00, 0x68, 0x01, 0x01, 0x00, 0xeb } },  // rwark-blue-slow
            { 6, new byte[] { 0x01, 0x0d, 0x00, 0x0d, 0x00, 0x50, 0x00, 0x78, 0x00, 0x78, 0x00, 0x00, 0x00, 0x64, 0x00, 0x01, 0x0a, 0x19 } },  // drain
            { 7, new byte[] { 0x01, 0x0d, 0x00, 0x0d, 0x00, 0x96, 0x00, 0x00, 0x00, 0x68, 0x01, 0x00, 0x02, 0x64, 0x00, 0x01, 0x0a, 0x19 } },  // drain-rnd
            { 8, new byte[] { 0x01, 0x0d, 0x00, 0x0d, 0x00, 0x96, 0x00, 0xf0, 0x00, 0x68, 0x01, 0x01, 0x00, 0x64, 0x00, 0x01, 0x0a, 0x14 } }   // drain-blue
        };

        public MainPage(TcpClientService tcpClient)
        {
            InitializeComponent();
            _tcpClient = tcpClient;
            _tcpClient.ConnectionStateChanged += OnConnectionStateChanged;
        }

        private void OnConnectionStateChanged(object? sender, bool isConnected)
        {
            MainThread.BeginInvokeOnMainThread(() =>
            {
                ConnectionStatusCheckBox.IsChecked = isConnected;
            });
        }

        private async void OnConnectClicked(object sender, EventArgs e)
        {
            if (_tcpClient.IsConnected)
            {
                // Disconnect if already connected
                _tcpClient.Disconnect();
                ConnectButton.Text = "Connect";
            }
            else
            {
                // Parse IP address and port
                var input = IpAddressEntry.Text?.Trim();
                if (string.IsNullOrEmpty(input))
                {
                    await DisplayAlert("Error", "Please enter IP address and port", "OK");
                    return;
                }

                var parts = input.Split(':');
                if (parts.Length != 2)
                {
                    await DisplayAlert("Error", "Invalid format. Use IP:Port (e.g., 192.168.1.10:1234)", "OK");
                    return;
                }

                if (!int.TryParse(parts[1], out int port))
                {
                    await DisplayAlert("Error", "Invalid port number", "OK");
                    return;
                }

                // Disable button during connection attempt
                ConnectButton.IsEnabled = false;
                ConnectButton.Text = "Connecting...";

                bool connected = await _tcpClient.ConnectAsync(parts[0], port);

                ConnectButton.IsEnabled = true;

                if (!connected)
                {
                    ConnectButton.Text = "Connect";
                    await DisplayAlert("Error", "Failed to connect to server", "OK");
                }
                else
                {
                    ConnectButton.Text = "Disconnect";
                }
            }
        }

        private async Task SendButtonCommand(int buttonNumber)
        {
            if (!_tcpClient.IsConnected)
            {
                await DisplayAlert("Error", "Not connected to server", "OK");
                return;
            }

            if (_buttonCommands.TryGetValue(buttonNumber, out byte[]? command))
            {
                bool success = await _tcpClient.SendDataAsync(command);
                if (!success)
                {
                    await DisplayAlert("Error", "Failed to send command", "OK");
                }
            }
        }

        private async void OnButton1Clicked(object sender, EventArgs e) => await SendButtonCommand(1);
        private async void OnButton2Clicked(object sender, EventArgs e) => await SendButtonCommand(2);
        private async void OnButton3Clicked(object sender, EventArgs e) => await SendButtonCommand(3);
        private async void OnButton4Clicked(object sender, EventArgs e) => await SendButtonCommand(4);
        private async void OnButton5Clicked(object sender, EventArgs e) => await SendButtonCommand(5);
        private async void OnButton6Clicked(object sender, EventArgs e) => await SendButtonCommand(6);
        private async void OnButton7Clicked(object sender, EventArgs e) => await SendButtonCommand(7);
        private async void OnButton8Clicked(object sender, EventArgs e) => await SendButtonCommand(8);
    }
}
