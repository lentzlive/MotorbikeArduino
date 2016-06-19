using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using Windows.Devices.Enumeration;
using Windows.Devices.Bluetooth.Rfcomm;
using Windows.Networking.Sockets;
using System.Collections.ObjectModel;
using Windows.Storage.Streams;
using Windows.UI.Popups;
using System.Threading.Tasks;
using System.Threading;
using System.Text;
using Windows.System.Threading;
using Windows.Storage;
//using RaspBlueTooth.Helpers;
using Windows.UI.Xaml.Media.Imaging;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace MotorbikeArduino
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {

        string Title = "Bluetooth Serial Comunication UWP by LentzLive";
        private Windows.Devices.Bluetooth.Rfcomm.RfcommDeviceService _service;
        private StreamSocket _socket;
        private DataWriter dataWriterObject;
        private DataReader dataReaderObject;
        ObservableCollection<PairedDeviceInfo> _pairedDevices;
        private CancellationTokenSource ReadCancellationTokenSource;

        string recvdtxt;
        private static ThreadPoolTimer timerDataProcess;

        public MainPage()
        {
            this.InitializeComponent();
            MyTitle.Text = Title;
            InitializeRfcommDeviceService();

        }

        async void InitializeRfcommDeviceService()
        {
            try
            {
                DeviceInformationCollection DeviceInfoCollection = await DeviceInformation.FindAllAsync(RfcommDeviceService.GetDeviceSelector(RfcommServiceId.SerialPort));

                var numDevices = DeviceInfoCollection.Count();

                // By clearing the backing data, we are effectively clearing the ListBox
                _pairedDevices = new ObservableCollection<PairedDeviceInfo>();
                _pairedDevices.Clear();

                if (numDevices == 0)
                {
                    //MessageDialog md = new MessageDialog("No paired devices found", "Title");
                    //await md.ShowAsync();
                    System.Diagnostics.Debug.WriteLine("InitializeRfcommDeviceService: No paired devices found.");
                }
                else
                {
                    // Found paired devices.
                    foreach (var deviceInfo in DeviceInfoCollection)
                    {
                        _pairedDevices.Add(new PairedDeviceInfo(deviceInfo));
                    }
                }
                PairedDevices.Source = _pairedDevices;
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine("InitializeRfcommDeviceService: " + ex.Message);
            }
        }

        #region Connessione Dispositivo

        async private void ConnectDevice_Click(object sender, RoutedEventArgs e)
        {
            //Revision: No need to requery for Device Information as we alraedy have it:
            DeviceInformation DeviceInfo; // = await DeviceInformation.CreateFromIdAsync(this.TxtBlock_SelectedID.Text);
            PairedDeviceInfo pairedDevice = (PairedDeviceInfo)ConnectDevices.SelectedItem;
            DeviceInfo = pairedDevice.DeviceInfo;

            bool success = true;
            try
            {
                _service = await RfcommDeviceService.FromIdAsync(DeviceInfo.Id);

                if (_socket != null)
                {
                    // Disposing the socket with close it and release all resources associated with the socket
                    _socket.Dispose();
                }

                _socket = new StreamSocket();
                try
                {
                    // Note: If either parameter is null or empty, the call will throw an exception
                    await _socket.ConnectAsync(_service.ConnectionHostName, _service.ConnectionServiceName);
                }
                catch (Exception ex)
                {
                    success = false;
                    System.Diagnostics.Debug.WriteLine("Connect:" + ex.Message);
                }
                // If the connection was successful, the RemoteAddress field will be populated
                if (success)
                {
                    this.buttonDisconnect.IsEnabled = true;
                    this.buttonSend.IsEnabled = true;
                    this.buttonStartRecv.IsEnabled = true;
                    this.buttonStartProcess.IsEnabled = true;
                    this.buttonStopRecv.IsEnabled = false;
                    this.StartStopReceive.IsEnabled = true;

                    string msg = String.Format("Connected to {0}!", _socket.Information.RemoteAddress.DisplayName);
                    //MessageDialog md = new MessageDialog(msg, Title);
                    System.Diagnostics.Debug.WriteLine(msg);
                    //await md.ShowAsync();
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine("Overall Connect: " + ex.Message);
                _socket.Dispose();
                _socket = null;
            }
        }

        private void ConnectDevices_DoubleTapped(object sender, DoubleTappedRoutedEventArgs e)
        {
            PairedDeviceInfo pairedDevice = (PairedDeviceInfo)ConnectDevices.SelectedItem;
      
            ConnectDevice_Click(sender, e);
        }

        #endregion

        private async void button_Click(object sender, RoutedEventArgs e)
        {
            //OutBuff = new Windows.Storage.Streams.Buffer(100);
            Button button = (Button)sender;
            if (button != null)
            {
                switch ((string)button.Content)
                {
                    case "Disconnect":
                        await this._socket.CancelIOAsync();
                        _socket.Dispose();
                        _socket = null;
                     
                        this.buttonDisconnect.IsEnabled = false;
                        this.buttonSend.IsEnabled = false;
                        this.buttonStartRecv.IsEnabled = false;
                        this.buttonStopRecv.IsEnabled = false;
                        this.StartStopReceive.IsEnabled = false;

                        break;
                    case "Send":
                        //await _socket.OutputStream.WriteAsync(OutBuff);
                        Send(this.textBoxSendText.Text);
                        this.textBoxSendText.Text = "";
                        break;
                    case "Clear Send":
                        this.textBoxRecvdText.Text = "";
                        recvdtxt = "";
                        break;
                    case "Start Recv":
                        this.buttonStartRecv.IsEnabled = false;
                        this.buttonStopRecv.IsEnabled = true;
                        Listen();
                        break;
                    case "Stop Recv":
                        this.buttonStartRecv.IsEnabled = false;
                        this.buttonStopRecv.IsEnabled = false;
                        CancelReadTask();
                        break;
                    case "Refresh":
                        InitializeRfcommDeviceService();
                        break;
                    case "Start Process":
                        timerDataProcess = ThreadPoolTimer.CreatePeriodicTimer(dataProcessTick, TimeSpan.FromMilliseconds(Convert.ToInt32(100)));
                        break;
                }
            }
        }

        private async void dataProcessTick(ThreadPoolTimer timer)
        {
            try
            {
               
               
                await txtValuefromBT.Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
                {
                    txtValuefromBT.Text = FromHexString(recvdtxt);// Convert.ToString(pm10);
                }
                );
                string newstr = recvdtxt.Substring(20, 20);
                recvdtxt = "";
            }
            catch (Exception e)
            { }
        }


        public async void Send(string msg)
        {
            try
            {
                if (_socket.OutputStream != null)
                {
                    // Create the DataWriter object and attach to OutputStream
                    dataWriterObject = new DataWriter(_socket.OutputStream);

                    //Launch the WriteAsync task to perform the write
                    await WriteAsync(msg);
                }
                else
                {
                    //status.Text = "Select a device and connect";
                }
                //System.Threading.Tasks.Task.Delay(500).Wait();
                //this.textBoxRecvdText.Text = recvdtxt;
                // textBoxPM10Text.Text = FromHexString(recvdtxt);

                // recvdtxt = "";
            }
            catch (Exception ex)
            {
                //status.Text = "Send(): " + ex.Message;
                System.Diagnostics.Debug.WriteLine("Send(): " + ex.Message);
            }
            finally
            {
                // Cleanup once complete
                if (dataWriterObject != null)
                {
                    dataWriterObject.DetachStream();
                    dataWriterObject = null;
                }
            }
        }

        /// <summary>
        /// WriteAsync: Task that asynchronously writes data from the input text box 'sendText' to the OutputStream 
        /// </summary>
        /// <returns></returns>
        private async Task WriteAsync(string msg)
        {
            Task<UInt32> storeAsyncTask;

            if (msg == "")
                msg = "none";// sendText.Text;
            if (msg.Length != 0)
            //if (msg.sendText.Text.Length != 0)
            {
                // Load the text from the sendText input text box to the dataWriter object
                dataWriterObject.WriteString(msg);

                // Launch an async task to complete the write operation
                storeAsyncTask = dataWriterObject.StoreAsync().AsTask();

                UInt32 bytesWritten = await storeAsyncTask;
                if (bytesWritten > 0)
                {
                    string status_Text = msg + ", ";
                    status_Text += bytesWritten.ToString();
                    status_Text += " bytes written successfully!";
                    System.Diagnostics.Debug.WriteLine(status_Text);
                }
            }
            else
            {
                string status_Text2 = "Enter the text you want to write and then click on 'WRITE'";
                System.Diagnostics.Debug.WriteLine(status_Text2);
            }
        }



        /// <summary>
        /// - Create a DataReader object
        /// - Create an async task to read from the SerialDevice InputStream
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private async void Listen()
        {
            try
            {
                ReadCancellationTokenSource = new CancellationTokenSource();
                if (_socket.InputStream != null)
                {
                    dataReaderObject = new DataReader(_socket.InputStream);
                    this.buttonStopRecv.IsEnabled = true;
                    this.buttonDisconnect.IsEnabled = false;
                    // keep reading the serial input
                    while (true)
                    {
                        await ReadAsync(ReadCancellationTokenSource.Token);
                    }
                }
                else
                {
                    recvdtxt = "";
                }
            }
            catch (Exception ex)
            {
                this.buttonStopRecv.IsEnabled = false;
                this.buttonStartRecv.IsEnabled = false;
                this.buttonSend.IsEnabled = false;
                this.buttonDisconnect.IsEnabled = false;
           
                if (ex.GetType().Name == "TaskCanceledException")
                {
                    System.Diagnostics.Debug.WriteLine("Listen: Reading task was cancelled, closing device and cleaning up");
                }
                else
                {
                    System.Diagnostics.Debug.WriteLine("Listen: " + ex.Message);
                }
            }
            finally
            {
                // Cleanup once complete
                if (dataReaderObject != null)
                {
                    dataReaderObject.DetachStream();
                    dataReaderObject = null;
                }
            }
        }

        /// <summary>
        /// ReadAsync: Task that waits on data and reads asynchronously from the serial device InputStream
        /// </summary>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        private async Task ReadAsync(CancellationToken cancellationToken)
        {
            Task<UInt32> loadAsyncTask;

            uint ReadBufferLength = 1024;

            // If task cancellation was requested, comply
            cancellationToken.ThrowIfCancellationRequested();

            // Set InputStreamOptions to complete the asynchronous read operation when one or more bytes is available
            dataReaderObject.InputStreamOptions = InputStreamOptions.Partial;

            // Create a task object to wait for data on the serialPort.InputStream
            loadAsyncTask = dataReaderObject.LoadAsync(ReadBufferLength).AsTask(cancellationToken);

            // Launch the task and wait
            UInt32 bytesRead = await loadAsyncTask;
            if (bytesRead > 0)
            {
                //byte readBuffer[]=new byte[dataReaderObject.ReadUInt32];
                byte[] fileContent = new byte[dataReaderObject.UnconsumedBufferLength];

                try
                {
                    dataReaderObject.ReadBytes(fileContent);
                    //string recvdtxt = Encoding.Unicode.GetString(fileContent, 0, fileContent.Length);             
                    //StringBuilder recBuffer16 = new StringBuilder();

                    for (int i = 0; i < fileContent.Length; i++)
                    {
                        string recvdtxt1 = fileContent[i].ToString("X2");
                        recvdtxt += recvdtxt1;
                    }
                    //dataReaderObject.ReadBytes();
                    System.Diagnostics.Debug.WriteLine(recvdtxt);
                    this.textBoxRecvdText.Text = recvdtxt;// FromHexString( recvdtxt);
                    txtValuefromBT.Text = FromHexString(this.textBoxRecvdText.Text);
                    //recvdtxt = "";
                    /*if (_Mode == Mode.JustConnected)
                    {
                        if (recvdtxt[0] == ArduinoLCDDisplay.keypad.BUTTON_SELECT_CHAR)
                        {
                            _Mode = Mode.Connected;

                            //Reset back to Cmd = Read sensor and First Sensor
                            await Globals.MP.UpdateText("@");
                            //LCD Display: Fist sensor and first comamnd
                            string lcdMsg = "~C" + Commands.Sensors[0];
                            lcdMsg += "~" + ArduinoLCDDisplay.LCD.CMD_DISPLAY_LINE_2_CH + Commands.CommandActions[1] + "           ";
                            Send(lcdMsg);

                            backButton_Click(null, null);
                        }
                    }
                    else if (_Mode == Mode.Connected)
                    {
                        await Globals.MP.UpdateText(recvdtxt);
                        recvdText.Text = "";
                        status.Text = "bytes read successfully!";
                    }*/

                    this.textBoxRecvdText.Text = "";
             
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine("ReadAsync: " + ex.Message);
                }

            }
        }

        public static string FromHexString(string hexString)
        {
            var bytes = new byte[hexString.Length / 2];
            for (var i = 0; i < bytes.Length; i++)
            {
                bytes[i] = Convert.ToByte(hexString.Substring(i * 2, 2), 16);
            }

            return Encoding.UTF8.GetString(bytes); // returns: "Hello world" for "48656C6C6F20776F726C64"
        }

        /// <summary>
        /// CancelReadTask:
        /// - Uses the ReadCancellationTokenSource to cancel read operations
        /// </summary>
        private void CancelReadTask()
        {
            if (ReadCancellationTokenSource != null)
            {
                if (!ReadCancellationTokenSource.IsCancellationRequested)
                {
                    ReadCancellationTokenSource.Cancel();
                    recvdtxt = "";
                }
            }
        }


        /// <summary>
        ///  Class to hold all paired device information
        /// </summary>
        public class PairedDeviceInfo
        {
            internal PairedDeviceInfo(DeviceInformation deviceInfo)
            {
                this.DeviceInfo = deviceInfo;
                this.ID = this.DeviceInfo.Id;
                this.Name = this.DeviceInfo.Name;
            }

            public string Name { get; private set; }
            public string ID { get; private set; }
            public DeviceInformation DeviceInfo { get; private set; }
        }

        private void StartStopReceive_Toggled(object sender, RoutedEventArgs e)
        {
            ToggleSwitch toggleSwitch = sender as ToggleSwitch;
            if (toggleSwitch != null)
            {
                if (toggleSwitch.IsOn == true)
                {
                    Listen();
                }
                else
                {
                    CancelReadTask();
                }
            }
        }

        private void StartProcess_Toggled(object sender, RoutedEventArgs e)
        {


            ToggleSwitch toggleSwitch = sender as ToggleSwitch;
            if (toggleSwitch != null)
            {
                if (toggleSwitch.IsOn == true)
                {
                    timerDataProcess = ThreadPoolTimer.CreatePeriodicTimer(dataProcessTick, TimeSpan.FromMilliseconds(Convert.ToInt32(100)));
                }
                else
                {
                    CancelReadTask();
                }
            }


        }

        private void tgsLightOn_Toggled(object sender, RoutedEventArgs e)
        {

        }

    }
}
