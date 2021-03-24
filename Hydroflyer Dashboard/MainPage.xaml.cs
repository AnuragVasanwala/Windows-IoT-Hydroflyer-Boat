using System;
using System.IO;
using System.Net;
using System.Threading.Tasks;
using Windows.Devices.Geolocation;
using Windows.Devices.Sensors;
using Windows.Graphics.Display;
using Windows.Storage.Streams;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Maps;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Navigation;

/*
  Windows Phone 8.1/10 Application for Windows 10 IoT Core - Hydroflyer
  https://www.hackster.io/AnuragVasanwala/windows-10-iot-core-hydroflyer-f83190
  
  Objectives:
    + Communicate with ESP8266-01 via WiFi Link
    + Convert accelerometer data into range of 90 - 110 and send to Boat to process it
    + Parse locational, heading and other data that came back from Boat and update UI according
*/

namespace HydroflyerArgumentsashboard
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        enum CurrentStateEnum
        {
            FW,
            RT,
            LT,
            Stopped
        }

        /* Default maneuver state */
        CurrentStateEnum CurrentState = CurrentStateEnum.Stopped;

        /* MapIcon object for boat and WP */
        MapIcon MapIcon_Hydroflyer;
        MapIcon MapIcon_WindowsPhone;

        /* Global Temp BasicGeoposition to avoid frequent object creation */
        BasicGeoposition _BasicGeoposition;
        BasicGeoposition _BasicGeoposition_WP;

        /* Arguments to be sent to ESP8266 via AP */
        byte[] Arguments = new byte[6];

        /* Accelerometer object */
        Accelerometer _Accelerometer;
        AccelerometerReading _Reading;

        /* WP built-in GPS */
        Geolocator _Geolocator;
        Geoposition MyPosition;

        /* Received data will be parsed into this struct */
        struct ReceivedDataStruct
        {
            public bool ErrorOccoured;
            public double Latitude, Longitude;
            public byte ValidityByte;
            public uint Heading;
            public uint LeakDetection_RAW, FloatSwitch_RAW;
        };

        ReceivedDataStruct ReceivedData;

        public MainPage()
        {
            this.InitializeComponent();

            /* Initialize Accelerometer and built-in GPS */
            _Accelerometer = Accelerometer.GetDefault();
            _Geolocator = new Geolocator();
            
            this.NavigationCacheMode = NavigationCacheMode.Required;

            /* Force landscape mode */
            DisplayInformation.AutoRotationPreferences = DisplayOrientations.Landscape;
            
            /* Default location to show boat while GPS is acquiring signals */
            _BasicGeoposition.Latitude = 21.201395;
            _BasicGeoposition.Longitude = 72.788782;

            /* Place hydroflyer icon on map */
            Geopoint NewGeopoint = new Geopoint(_BasicGeoposition);
            MapIcon_Hydroflyer = new MapIcon();
            MapIcon_Hydroflyer.Image = RandomAccessStreamReference.CreateFromUri(new Uri("ms-appx:///Resources/Images/GPS_OnMap_PinWith_Compass.png"));
            MapIcon_Hydroflyer.Location = NewGeopoint;
            MapIcon_Hydroflyer.Title = "Hydroflyer";
            MyMap.MapElements.Add(MapIcon_Hydroflyer);

            /* Place WP icon on map */
            MapIcon_WindowsPhone = new MapIcon();
            MapIcon_WindowsPhone.Image = RandomAccessStreamReference.CreateFromUri(new Uri("ms-appx:///Resources/Images/Img_WP.png"));
            MapIcon_WindowsPhone.Location = NewGeopoint;
            MapIcon_WindowsPhone.Title = "Me";
            MyMap.MapElements.Add(MapIcon_WindowsPhone);

            /* Set map's center-point and desired pitch */
            MyMap.Center = NewGeopoint;
            MyMap.DesiredPitch = 0;
            
            /* Timer to update UI */
            DispatcherTimer UpdateUI_Timer = new DispatcherTimer();
            UpdateUI_Timer.Tick += UpdateUI_Timer_Tick; ;
            UpdateUI_Timer.Interval = TimeSpan.FromMilliseconds(40);
            UpdateUI_Timer.Start();

            /* Create separate Task that communicates with boat */
            Task Communication_Task = new Task(Communication_Function);
            Communication_Task.Start();

            /* Separate task which updates WP location on long intervals */
            Task WindowsPhoneLocation_Task = new Task(windowsPhoneLocation_Function);
            WindowsPhoneLocation_Task.Start();
        }

        private async void windowsPhoneLocation_Function()
        {
            while (true)
            {
                try
                {
                    /* Get current position of WP */
                    MyPosition = await _Geolocator.GetGeopositionAsync();

                    /* Update position of WP on map and set center point to boats location */
                    Geopoint NewGeopoint = new Geopoint(_BasicGeoposition);
                    MyMap.Center = NewGeopoint;
                }
                catch (Exception)
                {
                    
                }

                await Task.Delay(15000);
            }
        }

        private async void Communication_Function()
        {
            while (true)
            {
                /* Read accelerometer */
                _Reading = _Accelerometer.GetCurrentReading();

                /* Parse accelerometer value that can be in the range of 90 to 110 */
                Arguments[0] = (byte)(((int)(_Reading.AccelerationX * 10)) + 100);
                Arguments[1] = (byte)(((int)(_Reading.AccelerationY * 10)) + 100);
                
                try
                {
                    /* Create http GET request with Arguments */
                    HttpWebRequest _httpClient = (HttpWebRequest)WebRequest.Create("http://192.168.4.1/Maneuver?a=" + Arguments[0].ToString() + "&b=" + Arguments[1].ToString() + "&c=" + Arguments[2].ToString() + "&d=" + Arguments[3].ToString() + "&e=" + Arguments[4].ToString() + "&f=" + Arguments[5].ToString());

                    _httpClient.Proxy = null;

                    /* Boat will send back string containing precious sensory data separated by '|' */
                    HttpWebResponse wr = (HttpWebResponse)_httpClient.GetResponseAsync().Result;
                    StreamReader sr = new StreamReader(wr.GetResponseStream());
                    string SensorData = sr.ReadToEnd();

                    /* GPS Valid | Latitude | Longitude | Heading | LeakDetection_Raw | FloatSwitch_Raws */
                    string[] Data = SensorData.Split('|');

                    byte.TryParse(Data[0], out ReceivedData.ValidityByte);
                    double.TryParse(Data[1], out ReceivedData.Latitude);
                    double.TryParse(Data[2], out ReceivedData.Longitude);
                    uint.TryParse(Data[3], out ReceivedData.Heading);
                    uint.TryParse(Data[4], out ReceivedData.LeakDetection_RAW);
                    uint.TryParse(Data[5], out ReceivedData.FloatSwitch_RAW);

                    ReceivedData.ErrorOccoured = false;
                }
                catch (Exception)
                {
                    ReceivedData.ErrorOccoured = true;
                }
                await Task.Delay(40);
            }
        }

        private void UpdateUI_Timer_Tick(object sender, object e)
        {
            /* Update map's heading to boat's heading */
            MyMap.Heading = ReceivedData.Heading;

            _BasicGeoposition.Latitude = ReceivedData.Latitude;
            _BasicGeoposition.Longitude = ReceivedData.Longitude;

            try
            {
                _BasicGeoposition_WP.Latitude = MyPosition.Coordinate.Latitude;
                _BasicGeoposition_WP.Longitude = MyPosition.Coordinate.Longitude;
            }
            catch (Exception)
            {
                
            }
            
            Geopoint NewGeopoint2 = new Geopoint(_BasicGeoposition_WP);
            MapIcon_WindowsPhone.Location = NewGeopoint2;

           
            /* Update maneuver indicator gauge */
            if (Arguments[0] > 98)
            {
                if (Arguments[1] > 102 && CurrentState!=CurrentStateEnum.LT)
                {
                    Img_FW.Source = new BitmapImage(new Uri("ms-appx:///Resources/Images/Maneuver/Img_FW0.png"));
                    Img_RT.Source = new BitmapImage(new Uri("ms-appx:///Resources/Images/Maneuver/Img_RT0.png"));
                    Img_Stop.Source = new BitmapImage(new Uri("ms-appx:///Resources/Images/Maneuver/Img_Stop0.png"));

                    Img_LT.Source = new BitmapImage(new Uri("ms-appx:///Resources/Images/Maneuver/Img_LT1.png"));
                    CurrentState = CurrentStateEnum.LT;
                }
                else if (Arguments[1] < 98 && CurrentState != CurrentStateEnum.RT)
                {
                    Img_LT.Source = new BitmapImage(new Uri("ms-appx:///Resources/Images/Maneuver/Img_LT0.png"));
                    Img_FW.Source = new BitmapImage(new Uri("ms-appx:///Resources/Images/Maneuver/Img_FW0.png"));
                    Img_Stop.Source = new BitmapImage(new Uri("ms-appx:///Resources/Images/Maneuver/Img_Stop0.png"));

                    Img_RT.Source = new BitmapImage(new Uri("ms-appx:///Resources/Images/Maneuver/Img_RT1.png"));
                    CurrentState = CurrentStateEnum.RT;
                }
                else if (CurrentState != CurrentStateEnum.FW)
                {
                    Img_LT.Source = new BitmapImage(new Uri("ms-appx:///Resources/Images/Maneuver/Img_LT0.png"));
                    Img_RT.Source = new BitmapImage(new Uri("ms-appx:///Resources/Images/Maneuver/Img_RT0.png"));
                    Img_Stop.Source = new BitmapImage(new Uri("ms-appx:///Resources/Images/Maneuver/Img_Stop0.png"));

                    Img_FW.Source = new BitmapImage(new Uri("ms-appx:///Resources/Images/Maneuver/Img_FW1.png"));
                    CurrentState = CurrentStateEnum.FW;
                }
            }
            else if ( CurrentState != CurrentStateEnum.Stopped)
            {
                Img_LT.Source = new BitmapImage(new Uri("ms-appx:///Resources/Images/Maneuver/Img_LT0.png"));
                Img_FW.Source = new BitmapImage(new Uri("ms-appx:///Resources/Images/Maneuver/Img_FW0.png"));
                Img_RT.Source = new BitmapImage(new Uri("ms-appx:///Resources/Images/Maneuver/Img_RT0.png"));
                
                Img_Stop.Source = new BitmapImage(new Uri("ms-appx:///Resources/Images/Maneuver/Img_Stop1.png"));
                CurrentState = CurrentStateEnum.Stopped;
            }

            /* Update legends */
            if (ReceivedData.ValidityByte==31)
            {
                Img_GPSStatus.Source = new BitmapImage(new Uri("ms-appx:///Resources/Images/Img_GPS_1.png"));
            }
            else
            {
                Img_GPSStatus.Source = new BitmapImage(new Uri("ms-appx:///Resources/Images/Img_GPS_0.png"));
            }

            if (ReceivedData.ErrorOccoured)
            {
                Img_LinkStatus.Source = new BitmapImage(new Uri("ms-appx:///Resources/Images/Img_NoLink.png"));
            }
            else
            {
                Img_LinkStatus.Source = new BitmapImage(new Uri("ms-appx:///Resources/Images/Img_Linked.png"));
            }

            if (ReceivedData.LeakDetection_RAW > 100 && ReceivedData.LeakDetection_RAW < 750)
            {
                Img_LeakStatus.Source = new BitmapImage(new Uri("ms-appx:///Resources/Images/Img_LeakDetected_BG.png"));
            }
            else
            {
                Img_LeakStatus.Source = new BitmapImage(new Uri("ms-appx:///Resources/Images/Img_NoLeak.png"));
            }

            if (ReceivedData.FloatSwitch_RAW > 100 && ReceivedData.FloatSwitch_RAW < 750)
            {
                Img_BoatStatus.Source = new BitmapImage(new Uri("ms-appx:///Resources/Images/Img_BoatOnWater.png"));
            }
            else
            {
                Img_BoatStatus.Source = new BitmapImage(new Uri("ms-appx:///Resources/Images/Img_BoatOutOfWater.png"));
            }
        }

        /// <summary>
        /// Invoked when this page is about to be displayed in a Frame.
        /// </summary>
        /// <param name="e">Event data that describes how this page was reached.
        /// This parameter is typically used to configure the page.</param>
        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            // TODO: Prepare page for display here.

            // TODO: If your application contains multiple pages, ensure that you are
            // handling the hardware Back button by registering for the
            // Windows.Phone.UI.Input.HardwareButtons.BackPressed event.
            // If you are using the NavigationHelper provided by some templates,
            // this event is handled for you.
        }
    }
}
