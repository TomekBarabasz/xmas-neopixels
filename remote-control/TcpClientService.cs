using System.Net.Sockets;
using System.Text;

namespace Xmas_Led_Ctrl;

public class TcpClientService
{
    private TcpClient? _client;
    private NetworkStream? _stream;
    private bool _isConnected;

    public event EventHandler<bool>? ConnectionStateChanged;

    public bool IsConnected => _isConnected;

    public async Task<bool> ConnectAsync(string ipAddress, int port)
    {
        try
        {
            _client = new TcpClient();
            await _client.ConnectAsync(ipAddress, port);
            _stream = _client.GetStream();
            _isConnected = true;
            ConnectionStateChanged?.Invoke(this, true);
            return true;
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Connection error: {ex.Message}");
            Disconnect();
            return false;
        }
    }

    public void Disconnect()
    {
        try
        {
            _stream?.Close();
            _client?.Close();
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Disconnect error: {ex.Message}");
        }
        finally
        {
            _stream = null;
            _client = null;
            _isConnected = false;
            ConnectionStateChanged?.Invoke(this, false);
        }
    }

    public async Task<bool> SendDataAsync(byte[] data)
    {
        if (!_isConnected || _stream == null)
            return false;

        try
        {
            await _stream.WriteAsync(data, 0, data.Length);
            return true;
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Send error: {ex.Message}");
            Disconnect();
            return false;
        }
    }
}
