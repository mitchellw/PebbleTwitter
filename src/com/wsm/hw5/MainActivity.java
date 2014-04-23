package com.wsm.hw5;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

import twitter4j.FilterQuery;
import twitter4j.StallWarning;
import twitter4j.Status;
import twitter4j.StatusDeletionNotice;
import twitter4j.StatusListener;
import twitter4j.TwitterStream;
import twitter4j.TwitterStreamFactory;
import twitter4j.conf.ConfigurationBuilder;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.text.Editable;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.ViewGroup.LayoutParams;
import android.widget.EditText;
import android.widget.RelativeLayout;
import android.widget.Toast;

import com.getpebble.android.kit.PebbleKit;
import com.getpebble.android.kit.PebbleKit.PebbleAckReceiver;
import com.getpebble.android.kit.PebbleKit.PebbleNackReceiver;
import com.getpebble.android.kit.util.PebbleDictionary;

public class MainActivity extends Activity {
	//~PEBBLE Transaction Ids--------------------------------------------------------------------------------------------------------
	/**
	 * The PebbleDictionary key value used to retrieve the identifying transactionId field.
	 */
	private static final int EVENT_ID_KEY = 79;

	/**
	 * different values stored with the EVENT_ID_KEY
	 */
	private static final int TWEET_REQUEST = 94;
	private static final int TWEET_SEND = 95;

	/**
	 * Key used to store the tweet
	 */
	private static final int TWEET_KEY = 37;
	/**
	 * Key used to store the author
	 */
	private static final int AUTHOR_KEY = 38;


	//~Constants---------------------------------------------------------------------------------------------------------------------
	/**
	 * String used to identify Log messages coming from this class
	 */
	private static final String TAG = "MainActivity";
	/**
	 * The UUID of the Pebble APP running on the Pebble.
	 */
	private static final UUID PEBBLE_APP_UUID = UUID.fromString("8e8a4feb-2704-4b72-872b-9cd51dc33d0b");

	private static final int MIN_TIME_TO_UPDATE = 2000;

	//~Variables--------------------------------------------------------------------------------------------------------------------
	private EditText mSearchEditText;
	private List<String> trackers;
	private FilterQuery query;
	private TwitterStream twitterStream;

	private long lastUpdated;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		RelativeLayout mLayout = new RelativeLayout(this);
		mLayout.setLayoutParams(
				new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));

		mSearchEditText = new EditText(this);
		RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(
				RelativeLayout.LayoutParams.MATCH_PARENT,
				RelativeLayout.LayoutParams.WRAP_CONTENT);
		params.addRule(RelativeLayout.CENTER_IN_PARENT);
		mSearchEditText.setLayoutParams(params);
		mLayout.addView(mSearchEditText);

		ConfigurationBuilder cb = new ConfigurationBuilder();
		TwitterStreamFactory tf = new TwitterStreamFactory(cb.build());
		twitterStream = tf.getInstance();

		trackers = new ArrayList<String>();
		trackers.add("#pebble");
		trackers.add("#tybg");
		trackers.add("#ratchet");
		trackers.add("#earthday");
		query = new FilterQuery();
		query.track(trackers.toArray(new String[]{}));
		twitterStream.addListener(new StatusListener() {
			@Override
			public void onException(Exception arg0) {}

			@Override
			public void onTrackLimitationNotice(int arg0) {}

			@Override
			public void onStallWarning(StallWarning arg0) {}

			@Override
			public void onScrubGeo(long arg0, long arg1) {}

			@Override
			public void onDeletionNotice(StatusDeletionNotice arg0) {}

			@Override
			public void onStatus(Status status) {
				sendTweetToPebble(new Tweet(status));
			}
		});

		setupPebbleCommunication();

		setContentView(mLayout);
	}
	
	@Override
	protected void onPause() {
		super.onPause();
		twitterStream.shutdown();
		twitterStream.cleanUp();
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		twitterStream.filter(query);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Handle action bar item clicks here. The action bar will
		// automatically handle clicks on the Home/Up button, so long
		// as you specify a parent activity in AndroidManifest.xml.
		int id = item.getItemId();
		if (id == R.id.action_settings) {
			return true;
		}
		return super.onOptionsItemSelected(item);
	}

	//~PEBBLE METHODS---------------------------------------------------------------------------------------
	/**
	 * Sets up all listeners and stuff to facilitate communication
	 * with the Pebble Watch.
	 */
	private void setupPebbleCommunication() {

		//Check if Pebble is already connected!
		if (PebbleKit.isWatchConnected(getApplicationContext())) {
			String messageString = "Your Pebble watch is connected!";
			Toast.makeText(this, messageString, Toast.LENGTH_SHORT).show();
			Log.i(TAG, messageString);        	
		}
		else {
			String messageString = "Your Pebble watch IS NOT connected! FIX THIS!";
			Toast.makeText(this, messageString, Toast.LENGTH_SHORT).show();
			Log.i(TAG, messageString);
		}

		//Listen for the pebble connection event
		PebbleKit.registerPebbleConnectedReceiver(this, new BroadcastReceiver() {

			@Override
			public void onReceive(Context context, Intent intent) {
				String messageString = "You just connected your Pebble Watch! Congrats!";
				Toast.makeText(context, messageString, Toast.LENGTH_SHORT).show();
				Log.i(TAG, messageString);

			}
		});

		//Listen for the pebble disconnected event
		PebbleKit.registerPebbleConnectedReceiver(this, new BroadcastReceiver() {

			@Override
			public void onReceive(Context context, Intent intent) {
				String messageString = "You just disconnected your Pebble Watch! Why you disconnect!??";				
				Toast.makeText(context, messageString, Toast.LENGTH_SHORT).show();
				Log.i(TAG, messageString);
			}
		});

		//Register to receive ACKS back from Pebble after sending message
		PebbleKit.registerReceivedAckHandler(this, new PebbleAckReceiver(PEBBLE_APP_UUID) {

			@Override
			public void receiveAck(Context context, int transactionId) {
				String messageString = "RECEIVED AN ACK FOR transactionId: " + transactionId;
				Toast.makeText(context, messageString, Toast.LENGTH_SHORT).show();
				Log.i(TAG, messageString);
			}
		});

		//Register to receive NACKS back from the Pebble after sending message
		PebbleKit.registerReceivedNackHandler(this, new PebbleNackReceiver(PEBBLE_APP_UUID) {

			@Override
			public void receiveNack(Context context, int transactionId) {
				String messageString = "RECEIVED A NACK FOR transactionId: " + transactionId;
				Toast.makeText(context, messageString, Toast.LENGTH_SHORT).show();
				Log.i(TAG, messageString);
			}
		});

		//Register to receive messages
		PebbleKit.registerReceivedDataHandler(this, new PebbleKit.PebbleDataReceiver(PEBBLE_APP_UUID) {

			@Override
			public void receiveData(Context context, int transactionId,	PebbleDictionary data) {
				PebbleKit.sendAckToPebble(context, transactionId);

				int myTransactionId = Long.valueOf(data.getInteger(EVENT_ID_KEY)).intValue();

				String messageString = "Received a message from the Pebble with myTransactionId == " + myTransactionId;
				Toast.makeText(context, messageString, Toast.LENGTH_SHORT).show();
				Log.i(TAG, messageString);

				switch (myTransactionId) {

				case TWEET_REQUEST:
					String newTracker = getSearchText();
					if (newTracker != null && newTracker.length() > 0) {
						//						if (newTracker.charAt(0) != '#') {
						//							newTracker = '#' + newTracker;
						//						}
						trackers.add(newTracker);
						query.track(trackers.toArray(new String[]{}));
					}
					sendTweetToPebble(null);
					break;

				default:
					Log.e(TAG, "Received an unknown transactionId: " + myTransactionId);
					break;
				}
			}
		});
	}

	private String getSearchText() {
		if (mSearchEditText == null) {
			return null;
		}

		Editable editable = mSearchEditText.getText();
		if (editable == null) {
			return null;
		}

		return editable.toString();
	}

	private void sendTweetToPebble(ITweet tweet) {
		if (tweet != null && System.currentTimeMillis() - lastUpdated > MIN_TIME_TO_UPDATE) {
			PebbleDictionary tweetListDict = new PebbleDictionary();

			//Add tweet to a PebbleDictionary
			tweetListDict.addBytes(AUTHOR_KEY, tweet.getUser().getBytes());
			tweetListDict.addBytes(TWEET_KEY, tweet.getText().getBytes());

			//Send the PebbleDictionary to the Pebble Watch app with PEBBLE_APP_UUID with the appropriate TransactionId
			PebbleKit.sendDataToPebbleWithTransactionId(this, PEBBLE_APP_UUID, tweetListDict, TWEET_SEND);
			Log.i(TAG, "Tweet to Pebble.......SENT!!!!!!!!!!!!!!!!!!!!");
			lastUpdated = System.currentTimeMillis();
		}
	}
}
