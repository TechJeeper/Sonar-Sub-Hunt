module.exports = [
  {
    type: 'heading',
    defaultValue: 'Sonar Sub Hunt'
  },
  {
    type: 'text',
    defaultValue: 'Choose how sonar pings and shots feel on your wrist.'
  },
  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Feedback'
      },
      {
        type: 'select',
        messageKey: 'FeedbackMode',
        label: 'Mode',
        defaultValue: '0',
        options: [
          { label: 'Both (sound + vibrate)', value: '0' },
          { label: 'Sound only', value: '1' },
          { label: 'Vibrate only', value: '2' }
        ]
      },
      {
        type: 'text',
        defaultValue: 'Watches without a speaker still vibrate when Both is selected.'
      }
    ]
  },
  {
    type: 'submit',
    defaultValue: 'Save'
  }
];
