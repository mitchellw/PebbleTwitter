package com.wsm.hw5;

import twitter4j.Status;

public class Tweet implements ITweet {
	private String username;
	private String text;

	public Tweet(Status status) {
		this.username = "@" + status.getUser().getScreenName();
		this.text = status.getText();
	}

	@Override
	public String getText() {
		return text;
	}

	@Override
	public String getUser() {
		return username;
	}

	public String toString() {
		return text;
	}
}
