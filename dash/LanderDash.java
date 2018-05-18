import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.text.*;
import javax.swing.border.*;
import java.io.*;
import static java.lang.Float.parseFloat;
import static java.lang.Integer.parseInt;
import java.net.*;
import java.util.*;

public class LanderDash extends JFrame implements Runnable {
    public static final long serialVersionUID = 2L;
    public static void main ( String[] args ) throws UnknownHostException {
        SwingUtilities.invokeLater( new Runnable() {
            public void run() {
                new LanderDash();
            }
        });
    }

    /* Information from Lander to Display */
    
    float fuel;
    float altitude;
    boolean flying, automatic;
    int vx, vy;
    
    FuelDisplay fuelDisplay = new FuelDisplay();
    AltitudeDisplay altitudeDisplay = new AltitudeDisplay();
    FlyingDisplay flyingDisplay = new FlyingDisplay();
    OrientationDisplay orientationDisplay = new OrientationDisplay();
    ControlDisplay controlDisplay = new ControlDisplay();
    
    /* Panel to display IP and port numnber */
    DatagramPanel connection = new DatagramPanel();


    public LanderDash(){
        super("Lunar Lander Dashboard");
        this.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

        getContentPane().setLayout(
            new BoxLayout(getContentPane(),BoxLayout.Y_AXIS) );
        
        fuel = 0;
        altitude = 0;
        
        add( connection ) ;
        add(flyingDisplay);
        add(controlDisplay);
        add(fuelDisplay);
        add(altitudeDisplay);
        add(orientationDisplay);

        pack();
        setVisible(true);
         (new Thread(this)).start();
    }

    public void run(){
        try {
            InetAddress addr = InetAddress.getLocalHost();
            int portno = 65250;
            DatagramSocket socket = new DatagramSocket(portno, addr);
            connection.setAddress((InetSocketAddress)socket.getLocalSocketAddress());
            while(true){

                /* set up socket for reception */
                if(socket!=null){
                /* start with fresh datagram packet */
                    byte[] buffer = new byte[1024];
                    DatagramPacket packet = new DatagramPacket(buffer, buffer.length);
                    socket.receive( packet );
                    /* extract message and pick appart into
                       lines and key:value pairs
                    */
                    String message = new String(packet.getData());
                    String[] lines = message.trim().split("\n");
                    for(String l : lines){
                        String[] pair = l.split(":");
                        if (pair[0].equals("altitude")){
                            altitude = parseFloat(pair[1]);
                            
                        }
                        else if(pair[0].equals("fuel")){
                            fuel = parseFloat(pair[1]);

                        }
                        else if(pair[0].equals("flying")){
                            if (parseInt(pair[1]) == 1){
                                flying = true;
                            }
                            else if(parseInt(pair[1]) == 0){
                                flying = false;
                            }
                        }
                        else if(pair[0].equals("oVx")){
                            vx = parseInt(pair[1]);
                        }
                        else if(pair[0].equals("oVy")){
                            vy = parseInt(pair[1]);
                        }
                        else if(pair[0].equals("automatic")){
                            if (parseInt(pair[1]) == 1){
                                automatic = true;
                            }
                            else if(parseInt(pair[1]) == 0){
                                automatic = false;
                            }
                        }
                        }
                        fuelDisplay.setFuel(fuel, flying);
                        altitudeDisplay.setAltitude(altitude, flying);
                        flyingDisplay.setFly(flying);
                        orientationDisplay.setOrientation(vx, vy);
                        controlDisplay.setMode(automatic);
                        }
                    
                
                try{Thread.sleep(50);}catch(InterruptedException e){}
            }
            }
        
        catch(Exception e) {
            System.err.println(e);
            System.err.println("in LanderDash.run()");
            System.exit(-1);
        }
    }
}
