<?php

//////////////////////////////
/* Admin Portion            */
//////////////////////////////
function kued_reminder_admin() {
	kued_reminder_automation();

  return array(
		'#markup' => t('This is the KUED Reminder Tool.<br /><a href="@settings">Settings</a> | <a href="@reports">Reports</a><br />Right now only the Reports section works.  I haven\'t found the need to add settings just yet.',
		array(
			'@settings' => url('admin/config/content/kued/kued_reminder/settings',array('absolute' => TRUE)),
			'@reports' => url('admin/config/content/kued/kued_reminder/reports',array('absolute' => TRUE)),
			))
	);
}

function kued_reminder_settings() {
  return array(
		'#markup' => t('<h2>This is the KUED Reminder Settings page</h2><p>There are no settings.  Get used to it.</p>')
	);
}

// Form to display the history of reminders
function kued_reminder_reports_form($form, $form_state) {

  //$form = array(); // Default the form..yes? no?

  // Build the search form wrapper
	$form['filter'] = array(
	  '#type' => 'fieldset',
	  '#title' => t('Filter Options'),
	  '#weight' => 5,
	  '#collapsible' => FALSE,
	  '#collapsed' => FALSE,	
	);
	// Just some notes about this weird interface
	$form['filter']['notes'] = array(
		'#markup' => t('<h2>Search for either email OR mobile number. Wildcard entries work.</h2>'),
	);
  $form['filter']['email'] = array(
    '#type' => 'textfield',
    '#title' => t('Search Email'),
    '#size' => 30,
  );
  $form['filter']['mobile'] = array(
    '#type' => 'textfield',
    '#title' => t('Search Mobile Number'),
    '#size' => 30,
  );
  $form['filter']['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Search By Show Title (max 100 results)'),
    '#size' => 50,
  );

  $form['filter']['nid'] = array(
    '#type' => 'textfield',
    '#title' => t('Search By Node ID'),
    '#size' => 30,
    '#attributes' => array(
        ' type' => 'number',
    ),
  );

  $form['filter']['date'] = array(
    '#type' => 'fieldset', 
    '#title' => t('Search by Airdate'),
    '#collapsible' => FALSE, 
    '#collapsed' => FALSE,
  );

  $form['filter']['date']['start_date'] = array(
    '#title' => t('Start Date'),
    '#type' => 'date_popup',
    '#date_format' => 'm/d/Y',
    '#date_year_range' => '-10:+1',
  );
  $form['filter']['date']['end_date'] = array(
    '#title' => t('End Date'),
    '#type' => 'date_popup',
    '#date_format' => 'm/d/Y',
    '#date_year_range' => '-10:+1',
  );

  $form['filter']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Filter'),
		'#submit' => array('filter_user_reminder_form_submit'),
  );
  $form['filter']['reset'] = array(
    '#type' => 'submit',
    '#value' => t('Reset'),
    '#submit' => array('kued_reminder_filter_reset'),
  );
/*
  $form['filter']['export'] = array(
    '#type' => 'submit',
    '#value' => t('Export These Results (not working yet)'),
    '#submit' => array('kued_reminder_filter_export'),
  );
*/
	$form['filter']['more_notes'] = array(
		'#markup' => t('<div><strong>Also</strong>, if you want to use the table sorting tools select your sorting options by clicking on the table headers and THEN perform your search. Yeah, it\'s weird. If you don\'t like it hire a new web developer.</div>'),
	);

  // Build the sortable table header.
  $header = array(
    'rid' => array('data' => t('ID'), 'field' => 'rid'),
		'blocked' => array('data' => t('Blocked?')),
		'processed' => array('data' => t('Sent?'), 'field' => 'processed'),
		'series_name' => array('data' => t('Series')),
    'show' => array('data' => t('Show'), 'field' => 'episode_name'),
    'channel_tid' => array('data' => t('Channel'), 'field' => 'channel_tid'),
		'airdate' => array('data' => t('Airdate'), 'field' => 'airdate'),
    'reminder_time' => array('data' => t('Reminder Time'), 'field' => 'reminder_time'),
		'message_type' => array('data' => t('Type'), 'field' => 'type'),
		'email_address' => array('data' => t('Email')),
		'mobile_number' => array('data' => t('Mobile')),
  );

  //Build the rows.
  $options = array();
	$disabled = array();
	$reminder_group = array();
  $blocked = ''; // Empty the blocked variable for reuse

  // Check if there is sorting request
  if(isset($_GET['sort']) && isset($_GET['order'])){
    // Sort it Ascending or Descending?
    if($_GET['sort'] == 'asc')
      $sort = 'ASC';
    else
      $sort = 'DESC';
    // Which column will be sorted
     switch($_GET['order']){
       case 'Sent?':
         $order = 'processed';
         break;
       case 'Airdate':
         $order = 'airdate';
         break;
       case 'Reminder Time':
         $order = 'reminder_time';
         break;
       case 'Type':
         $order = 'type';
         break;
       case 'Channel':
         $order = 'channel_tid';
         break;
       case 'Show':
         $order = 'episode_name';
         break;
       default:
         $order = 'rid';
     }
   } else {
     // Default sort
     $sort = 'DESC';
     $order = ' rid';
   }

    //dpm($form_state);

    $limit = 100;
    // We need to a limit on results for single character searches    
    if(isset($form_state)) {
      if(strlen($form_state['values']['title']) <= 2) {
        $limit = 0;
        if(!empty($form_state['input']) && !empty($form_state['values']['title'])) {
          drupal_set_message(t('You\'ve only entered two or fewer character in your search. Please enter more characters. Don\'t overload our database.'));
        }
      }
    }
    // If we're searching by show title we need to do a  lookup on the Show Title here
    $t = 1;
    if(isset($form_state['values']['title'])) {
      // Build a query to search the Node table for the title.
      $query = db_select('node', 'n')
        ->fields('n')
        ->condition('title', '%' . db_like($form_state['values']['title']) . '%', 'LIKE')
        ->distinct();
        $query->range(0, $limit);
        $result = $query->execute();

      // Build the array of NIDs
      while($titles = $result->fetchField()) {
        // We need a hard limit or the site will crash.
        if($t <= $limit) {
          $title[] = $titles;
        }
        $t++;
      }
    }

	// Here is the main query to collect the data for the table
	$query = db_select('kued_reminder', 'r')
    //->extend('PagerDefault')
		->fields('r')
    ->orderBy($order, $sort);

    // Modify using the query with the filter input
    if(isset($form_state['filters']['email'])) {
      $query->condition('r.email_address', '%' . db_like($form_state['filters']['email']) . '%', 'LIKE');
    }
    if(isset($form_state['filters']['mobile'])) {
      $query->condition('r.mobile_number', '%' . db_like($form_state['filters']['mobile']) . '%', 'LIKE');
    }
    if(!empty($form_state['values']['nid'])) {
      //dpm($form_state['values']['nid']);
      $query->condition('r.nid', $form_state['values']['nid']);
    }
    

    // Do the date search here
    if(isset($form_state['values']['start_date']) || isset($form_state['values']['end_date'])) {
      if(isset($form_state['values']['start_date']) && isset($form_state['values']['end_date'])) {
        // Format the date to a timestamp
        $start_date = strtotime($form_state['values']['start_date'] . ' 00:00:00');
        $end_date = strtotime($form_state['values']['end_date'] . ' 00:00:00');
        // Check to make sure they're in the correct order.
        if($start_date > $end_date) {
          // If not give a warning.
          drupal_set_message(t('The start date needs to come before the end date.'), 'warning');
        } else {
          // The dates are in the correct order, build the query.
          //dpm('do the date query');
          //dpm($start_date);
          //dpm($end_date);
          $query->condition('airdate', array($start_date, $end_date), 'BETWEEN');
        }
      } else {
        // Need both dates to search a range, show a warning.
        drupal_set_message(t('You\'ll need both the start and end dates to do a date range search.'), 'warning');
      }
    }

    // If we're searching by show title we need to do a reverse lookup on the NID here
    if(!empty($form_state['values']['title'])) {
      if(!empty($title)) {
        //Search for the show title via the NID
        //dpm('Title search');
        //dpm($title);
        $query->condition('r.nid', $title, 'IN');
      }
    }

    // Adjust the number of results so that we don't show everything if there isn't a filter applied
    if(empty($form_state['filters']['email']) || empty($form_state['filters']['mobile']) || empty($form_state['values']['title'])) {
     $query->range(0, 50);
    } elseif(!empty($form_state['values']['title'])) {
     $query->range(0, 50);
    }
  // Okay run the query
	$result = $query->execute();
  $current = $result->rowCount(); // current number of results

  // Built the results list to display
	while($record = $result->fetchAssoc()) {
    $reminder_group[] = $record;
  }

	// Get the total number of reminders in the DB

	$query = db_select('kued_reminder', 'r')
		->fields('r')
    //->extend('PagerDefault')
    ->extend('TableSort')
    ->orderByHeader($header);
	$count = $query->execute();
  $total = $count->rowCount();

  foreach($reminder_group as $item) {
    // check to see if the selected user(s) are already blocked
    $query = db_select('kued_reminder_blocked', 'r')
      ->fields('r')
      ->condition('email_address', $item['email_address'])
      ->execute();
    $check_one = $query->fetchField();

    // check to see if the selected user(s) are already blocked
    $query = db_select('kued_reminder_blocked', 'r')
      ->fields('r')
      ->condition('mobile_number', $item['mobile_number'])
      ->execute();
    $check_two = $query->fetchField();

    // Lets pretty up the results...........
    // See if the user is already blocked
    if(!empty($check_one) || !empty($check_two)) {
      $blocked = '<strong>Yes</strong>';
    } else {
      $blocked = 'No';
    }
    // build the nice looking path
    $path = drupal_get_path_alias('node/' . $item['nid']);
    $path = '<a href="/' . $path . '">' . $item['episode_name'] . '</a> (<a href="/node/' . $item['nid'] . '/edit">edit</a> | <strong>' . $item['nid'] . ')';
    // Format the processed variable to be readable
    $processed = '';
    if($item['processed'] == 'Y') {
      $processed = 'Yes';
    } else {
      $processed = 'No';
    }
    // Get the channel name from the TID
    $channel = taxonomy_term_load($item['channel_tid']);
    $channel = $channel->name;
    // Format the message_type variable to be readable 
    $type = '';
    if($item['message_type'] == 'text_message') {
      $type = 'Text';
    } else {
      $type = 'Email';
    }    
    // Format the email address
    $email = '';
    if(strlen($item['email_address'])) {
      $email = '<a href="mailto:' . $item['email_address'] . '">' . $item['email_address'] . '</a>'; 
    }

		// build the options for the table
		$options[$item['rid']] = array(
			'rid' => $item['rid'],
			'blocked' => $blocked,
			'processed' => $processed,
			'series_name' => $item['series_name'],
			'show' => $path,
			'channel_tid' => $channel,
 			'airdate' => format_date($item['airdate']),
			'reminder_time' => format_date($item['reminder_time']),
			'message_type' => $type,
			'email_address' => $email,
			'mobile_number' => $item['mobile_number'],
		);
    
    $path = ''; // Empty the path variable so it can be reused
  }

  // Build the form wrapper
	$form['reminder_report'] = array(
	  '#type' => 'fieldset',
	  '#title' => t('Reminder signup history and blocking tool'),
	  '#weight' => 5,
	  '#collapsible' => FALSE,
	  '#collapsed' => FALSE,	
    '#markup' => theme('pager')
	);
	// Clear all the import records
	$form['reminder_report']['total'] = array(
		'#markup' => t('<h2><strong>Showing :current results</strong> | :total total reminders</h2>', array(':current' => $current, ':total' => $total)),
	);
  //Build the tableselect.
  $form['reminder_report']['table'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $options,
    '#empty' => t('No results available.'),
		'#js_select' => FALSE,
		'#disabled' => FALSE,
  );
	// Block users from future updates
	$form['reminder_report']['block_user'] = array(
		'#type' => 'submit',
		'#submit' => array('block_user_reminder_form_submit'),
		'#value' => t('Block Users'),
	);
	// Block users from future updates
	$form['reminder_report']['unblock_user'] = array(
		'#type' => 'submit',
		'#submit' => array('unblock_user_reminder_form_submit'),
		'#value' => t('Unblock Users'),
	);

	// Block users from future updates
	$form['reminder_report']['delete'] = array(
		'#type' => 'submit',
		'#submit' => array('delete_reminder_form_submit'),
		'#value' => t('Delete Selected Reminders'),
	);

	// Warning
	$form['reminder_report']['notes'] = array(
		'#markup' => t('<div><strong>Note</strong>: Selected user\'s phone or email address will be blocked from future reminders.</div>'),
	);
  return $form;
}

/**
 * Passes search reset
 */
 function kued_reminder_filter_reset($form, &$form_state) {
  $form_state['rebuild'] = FALSE;
}


/*

// Form to display the history of reminders
function kued_reminder_blacklist_form($form, $form_state) {

  $form = array(); // Default the form..yes? no?

  // Build the search form wrapper
	$form['filter'] = array(
	  '#type' => 'fieldset',
	  '#title' => t('Blacklist'),
	  '#weight' => 5,
	  '#collapsible' => FALSE,
	  '#collapsed' => FALSE,
	);
	// Just some notes about this weird interface
	$form['filter']['notes'] = array(
		'#markup' => t('<h2>This form will allow you to blacklist phone numbers and email addresses.</h2>'),
	);
  $form['filter']['email'] = array(
    '#type' => 'textfield',
    '#title' => t('Block Email Address'),
    '#size' => 30,
  );
  $form['filter']['mobile'] = array(
    '#type' => 'textfield',
    '#title' => t('Block Mobile Number'),
    '#size' => 30,
  );
  $form['filter']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Add'),
		'#submit' => array('blacklist_reminder_form_submit'),
  );


	$form['filter']['more_notes'] = array(
		'#markup' => t('<div><strong>Also</strong>, if you want to use the table sorting tools select your sorting options by clicking on the table headers and THEN perform your search. Yeah, it\'s weird. If you don\'t like it hire a new web developer.</div>'),
	);

  // Build the sortable table header.
  $header = array(
    'rid' => array('data' => t('ID'), 'field' => 'rid'),
		'blocked' => array('data' => t('Blocked?')),
		'processed' => array('data' => t('Sent?'), 'field' => 'processed'),
		'series_name' => array('data' => t('Series')),
    'show' => array('data' => t('Show'), 'field' => 'episode_name'),
    'channel_tid' => array('data' => t('Channel'), 'field' => 'channel_tid'),
		'airdate' => array('data' => t('Airdate'), 'field' => 'airdate'),
    'reminder_time' => array('data' => t('Reminder Time'), 'field' => 'reminder_time'),
		'message_type' => array('data' => t('Type'), 'field' => 'type'),
		'email_address' => array('data' => t('Email')),
		'mobile_number' => array('data' => t('Mobile')),
  );

  //Build the rows.
  $options = array();
	$disabled = array();
	$reminder_group = array();
  $blocked = ''; // Empty the blocked variable for reuse

  // Check if there is sorting request
  if(isset($_GET['sort']) && isset($_GET['order'])){
    // Sort it Ascending or Descending?
    if($_GET['sort'] == 'asc')
      $sort = 'ASC';
    else
      $sort = 'DESC';
    // Which column will be sorted
     switch($_GET['order']){
       case 'Sent?':
         $order = 'processed';
         break;
       case 'Airdate':
         $order = 'airdate';
         break;
       case 'Reminder Time':
         $order = 'reminder_time';
         break;
       case 'Type':
         $order = 'type';
         break;
       case 'Channel':
         $order = 'channel_tid';
         break;
       case 'Show':
         $order = 'episode_name';
         break;
       default:
         $order = 'rid';
     }
   } else {
     // Default sort
     $sort = 'ASC';
     $order = ' rid';
   }

	// Here is the main query to collect the data for the table
	$query = db_select('kued_reminder', 'r')
    //->extend('PagerDefault')
		->fields('r')
    ->orderBy($order, $sort);

    // Modify using the query with the filter input
    if(isset($form_state['filters']['email'])) {
      $query->condition('r.email_address', '%' . db_like($form_state['filters']['email']) . '%', 'LIKE');
    }
    if(isset($form_state['filters']['mobile'])) {
      $query->condition('r.mobile_number', '%' . db_like($form_state['filters']['mobile']) . '%', 'LIKE');
    }
    // Adjust the number of results so that we don't show everything if there isn't a filter applied
    if(empty($form_state['filters']['email']) || empty($form_state['filters']['mobile'])) {
     $query->range(0,10);
    }
  // Okay run the query
	$result = $query->execute();
  $current = $result->rowCount(); // current number of results

  // Built the results list to display
	while($record = $result->fetchAssoc()) {
    $reminder_group[] = $record;
  }

	// Get the total number of reminders in the DB
	$query = db_select('kued_reminder', 'r')
		->fields('r');
	$count = $query->execute();
  $total = $count->rowCount();

  foreach($reminder_group as $item) {
    // check to see if the selected user(s) are already blocked
    $query = db_select('kued_reminder_blocked', 'r')
      ->fields('r')
      ->condition('email_address', $item['email_address'])
      ->execute();
    $check_one = $query->fetchField();

    // check to see if the selected user(s) are already blocked
    $query = db_select('kued_reminder_blocked', 'r')
      ->fields('r')
      ->condition('mobile_number', $item['mobile_number'])
      ->execute();
    $check_two = $query->fetchField();

    // Lets pretty up the results...........
    // See if the user is already blocked
    if(!empty($check_one) || !empty($check_two)) {
      $blocked = '<strong>Yes</strong>';
    } else {
      $blocked = 'No';
    }
    // build the nice looking path
    $path = drupal_get_path_alias('node/' . $item['nid']);
    $path = '<a href="/' . $path . '">' . $item['episode_name'] . '</a> (<a href="/node/' . $item['nid'] . '/edit">edit</a>)';
    // Format the processed variable to be readable
    $processed = '';
    if($item['processed'] == 'Y') {
      $processed = 'Yes';
    } else {
      $processed = 'No';
    }
    // Get the channel name from the TID
    $channel = taxonomy_term_load($item['channel_tid']);
    $channel = $channel->name;
    // Format the message_type variable to be readable 
    $type = '';
    if($item['message_type'] == 'text_message') {
      $type = 'Text';
    } else {
      $type = 'Email';
    }    
    // Format the email address
    $email = '';
    if(strlen($item['email_address'])) {
      $email = '<a href="mailto:' . $item['email_address'] . '">' . $item['email_address'] . '</a>'; 
    }

		// build the options for the table
		$options[$item['rid']] = array(
			'rid' => $item['rid'],
			'blocked' => $blocked,
			'processed' => $processed,
			'series_name' => $item['series_name'],
			'show' => $path,
			'channel_tid' => $channel,
 			'airdate' => format_date($item['airdate']),
			'reminder_time' => format_date($item['reminder_time']),
			'message_type' => $type,
			'email_address' => $email,
			'mobile_number' => $item['mobile_number'],
		);
    
    $path = ''; // Empty the path variable so it can be reused
  }

  // Build the form wrapper
	$form['reminder_report'] = array(
	  '#type' => 'fieldset',
	  '#title' => t('Reminder signup history and blocking tool'),
	  '#weight' => 5,
	  '#collapsible' => FALSE,
	  '#collapsed' => FALSE,	
	);
	// Clear all the import records
	$form['reminder_report']['total'] = array(
		'#markup' => t('<h2><strong>Showing</strong> :current of :total total</h2>', array(':current' => $current, ':total' => $total)),
	);
  //Build the tableselect.
  $form['reminder_report']['table'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $options,
    '#empty' => t('No results available.'),
		'#js_select' => FALSE,
		'#disabled' => FALSE,
  );
	// Block users from future updates
	$form['reminder_report']['block_user'] = array(
		'#type' => 'submit',
		'#submit' => array('block_user_reminder_form_submit'),
		'#value' => t('Block Users'),
	);
	// Block users from future updates
	$form['reminder_report']['unblock_user'] = array(
		'#type' => 'submit',
		'#submit' => array('unblock_user_reminder_form_submit'),
		'#value' => t('Unblock Users'),
	);
	// Warning
	$form['reminder_report']['notes'] = array(
		'#markup' => t('<div><strong>Note</strong>: Selected user\'s phone or email address will be blocked from future reminders.</div>'),
	);


  return $form;
}
*/

// Filter form handler
function filter_user_reminder_form_submit($form, &$form_state) {
  $form_state['filters']['email'] = $form_state['values']['email'];
  $form_state['filters']['mobile'] = $form_state['values']['mobile'];
  $form_state['rebuild'] = TRUE;
}

// Delete Reminders
function delete_reminder_form_submit($form, &$form_state) {
  if($form_state['values']['op'] == 'Delete Selected Reminders') {
    //dpm($form_state);
    $n = 0;
    foreach($form_state['values']['table'] as $key => $rid) {
      if($rid) {
        // Delete the reminder
        //dpm($rid);
        $deleted = db_delete('kued_reminder')
          ->condition('rid', $rid)
          ->execute();
          $n++;
      }
    }
    // If nothing was selected don't do anything
    // But tell the user that nothing happened.
    //dpm($deleted);
    //dpm($n);
    if(!strlen($deleted)) {
      drupal_set_message(t('No reminders were selected, nothing was deleted.'), 'warning');
    } else {
      drupal_set_message(t(':num_deleted reminders were deleted. I hope you meant to do this, there\'s no going back...', array(':num_deleted' => $n)), 'warning');
    }
  }
}

// Block an user by phone number or by email address
function block_user_reminder_form_submit($form, &$form_state) {
  $group = array();
  $check = '';
  // First grab an array of
  foreach($form_state['values']['table'] as $key => $row) {
    if($row) {
      // check get the Reminder ID and mobile/email to block this user
      $query = db_select('kued_reminder', 'r')
        ->fields('r')
        ->condition('rid', $row)
        ->execute();
      $result = $query->fetchAssoc();

      // Now do the dirty work...block this viewer's email or phone
      if(!empty($result['email_address'])) {
        // check to see if the selected user(s) are already blocked
        $query = db_select('kued_reminder_blocked', 'r')
          ->fields('r')
          ->condition('email_address', $result['email_address'])
          ->execute();
        $check = $query->fetchField();
        // If they're not blocked then insert the email address
        if(empty($check)) {
          //db_insert('kued_reminder_blocked', array('return' => Database::RETURN_NULL))
          db_insert('kued_reminder_blocked')
          ->fields(array(
            'date_sent' => REQUEST_TIME,
            'email_address' => $result['email_address'],
          ))
          ->execute();
          drupal_set_message(t('This email address was blocked - :email', array(':email' => $result['email_address']))); 
        } else {
          drupal_set_message(t('This email address is already blocked - :email', array(':email' => $result['email_address']))); 
        }
      } elseif(!empty($result['mobile_number'])) {
        // check to see if the selected user(s) are already blocked
        $query = db_select('kued_reminder_blocked', 'r')
          ->fields('r')
          ->condition('mobile_number', $result['mobile_number'])
          ->execute();
        $check = $query->fetchField();
        // If they're not blocked then insert the mobile number
        if(empty($check)) {
          db_insert('kued_reminder_blocked')
          ->fields(array(
            'date_sent' => REQUEST_TIME,
            'mobile_number' => $result['mobile_number'],
          ))
          ->execute();
          drupal_set_message(t('This mobile number was blocked - :mobile', array(':mobile' => $result['mobile_number']))); 
        } else {
          drupal_set_message(t('This mobile number is already blocked - :mobile', array(':mobile' => $result['mobile_number']))); 
        }
      }
    }
  }
}

// Remove blockages for user email or phone numbers
function unblock_user_reminder_form_submit($form, &$form_state) {
  $group = array();
  // First grab an array of
  foreach($form_state['values']['table'] as $key => $row) {
    if($row) {
      // check get the Reminder ID and mobile/email to UNblock this user
      $query = db_select('kued_reminder', 'r')
        ->fields('r')
        ->condition('rid', $row)
        ->execute();
      $result = $query->fetchAssoc();
      // Now UNblock this viewer's email or phone
      if(!empty($result['email_address'])) {
        db_delete('kued_reminder_blocked')
          ->condition('email_address', $result['email_address'])
          ->execute();
          drupal_set_message(t('This email address was unblocked - :email', array(':email' => $result['email_address']))); 
      } elseif(!empty($result['mobile_number'])) {
        db_delete('kued_reminder_blocked')
          ->condition('mobile_number', $result['mobile_number'])
          ->execute();
          drupal_set_message(t('This mobile number was unblocked - :mobile', array(':mobile' => $result['mobile_number']))); 
      }
    }
  }
}
