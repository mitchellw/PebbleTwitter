package com.wsm.hw5;

import java.io.IOException;
import java.util.List;
import java.util.UUID;

import twitter4j.Query;

import com.getpebble.android.kit.PebbleKit;
import com.getpebble.android.kit.PebbleKit.PebbleAckReceiver;
import com.getpebble.android.kit.PebbleKit.PebbleNackReceiver;
import com.getpebble.android.kit.util.PebbleDictionary;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.hardware.Camera.PictureCallback;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.TextureView;
import android.view.TextureView.SurfaceTextureListener;
import android.view.View;
import android.view.View.OnTouchListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.EditText;
import android.widget.RelativeLayout;
import android.widget.Toast;

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
	 * Key used to store the size of the tweets list
	 */
	private static final int LIST_SIZE_KEY = 37;


	//~Constants---------------------------------------------------------------------------------------------------------------------
	/**
	 * String used to identify Log messages coming from this class
	 */
	private static final String TAG = "MainActivity";
	/**
	 * The UUID of the Pebble APP running on the Pebble.
	 */
	private static final UUID PEBBLE_APP_UUID = UUID.fromString("8e8a4feb-2704-4b72-872b-9cd51dc33d0b");

	//~Variables--------------------------------------------------------------------------------------------------------------------
	private EditText mSearchEditText;

	private List<ITweet> tweetList;

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

		setupPebbleCommunication();

		setContentView(mLayout);
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
					sendTweetsToPebble();
					break;

				default:
					Log.e(TAG, "Received an unknown transactionId: " + myTransactionId);
					break;
				}
			}
		});
	}

	private void sendTweetsToPebble() {
		//Add all items from the list to a PebbleDictionary
		PebbleDictionary tweetListDict = new PebbleDictionary();
		byte[] bytes;
		int j;

		//Store list size in a PebbleDictionary
		tweetListDict.addUint32(LIST_SIZE_KEY, tweetList.size());
		//Add all items from the list to a PebbleDictionary
		for (int i = 0; i < tweetList.size(); i++) {
			//Store each item
			bytes = new byte[groceryList.get(i).getBytes().length + 1];
			byte[] tempBytes = groceryList.get(i).getBytes();
			for (j = 0; j < bytes.length - 1; j++) {

				bytes[j] = tempBytes[j];
			}
			//Store the status of each item's checkbox
			bytes[j] = (checkBox.isChecked()) ? (byte) (1 & 0xFF) : (byte) (0 & 0xFF);

			groceryListDict.addBytes(i, bytes);
		}

		//Send the PebbleDictionary to the Pebble Watch app with PEBBLE_APP_UUID with the appropriate TransactionId
		PebbleKit.sendDataToPebbleWithTransactionId(this, PEBBLE_APP_UUID, groceryListDict, GROCERY_LIST_SEND);
		Log.i(TAG, "Grocery list to Pebble.......SENT!!!!!!!!!!!!!!!!!!!!");
	}

	private class TwitterTask extends AsyncTask<Void, Void, List<ITweet>> {

		@Override
		protected List<ITweet> doInBackground(Void... arg0) {
			Query query = null;
			if (lastResult == null) {
				query = new Query(QUERY);
			}
			else {
				if (!lastResult.hasNext()) {
					return null;
				}
				query = lastResult.nextQuery();
			}

			List<IPhotoStreamItem> photoStream = new ArrayList<IPhotoStreamItem>();
			try {
				lastResult = twitter.search(query);
			}
			catch (TwitterException e) {
				Log.d(TAG, "Problem searching for tweets.");
				return photoStream;
			}

			if (lastResult != null) {
				List<twitter4j.Status> statuses = lastResult.getTweets();
				if (statuses != null) {
					for (twitter4j.Status status : statuses) {
						photoStream.add(new TwitterPhotoStreamItem(status));
					}
				}
			}

			return photoStream;
		}

		@Override
		protected void onPostExecute(List<IPhotoStreamItem> photoStream) {
			super.onPostExecute(photoStream);

			if (photoStream != null) {
				adapter.addAll(photoStream);
				adapter.notifyDataSetChanged();

				Date date = new Date(System.currentTimeMillis());

				String message = "Last updated at " + DateFormat.getDateTimeInstance().format(date);
				listView.onRefreshComplete(message);
				listView.onScrollToBottomComplete(message);

				listView.setOnScrollToBottomListener(new OnScrollToBottomListener() {

					@Override
					public void onScrollToBottom() {
						new TwitterTask().execute();
					}
				});
			}
		}
	}
}
